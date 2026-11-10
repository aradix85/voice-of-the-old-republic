// The engine's sub-screen and pause machinery: the overlay pause for the mod's
// own menus, the unpause safety net when a popup or the last sub-screen
// closes, the "Paused" / "Resumed" speech, and the hooks on CGuiInGame's
// Switch/Hide/SetSWGuiStatus and the server's SetPauseState (both games).

#include "engine_subscreen.h"

#include "engine_app.h"      // GetAppManager, GetServerApp, GetClientApp
#include "engine_game.h"
#include "engine_manager.h"  // kAddrGuiManagerPtr, kMgrModalStackSizeOffset
#include "engine_panels.h"   // HasActiveSubScreen, CallPrevSWInGameGui
#include "engine_reads.h"
#include "engine_rebase.h"
#include "log.h"
#include "menus.h"           // ClearPendingAnnounce
#include "menus_chain.h"     // InvalidateChain
#include "speech.h"
#include "strings.h"

#include <windows.h>

#include <cstdint>
#include <cstdio>

namespace acc::engine {

bool g_switchHookEverFired = false;

namespace {

// CServerExoApp::SetPauseState(mask, on_off), this = the server app. The
// mask is the bit itself; bit 2 is the menu/manual pause source that the
// pause key's TogglePauseRequest and every sub-screen and popup use. Unlike
// the toggle, setting is idempotent, so an unpause on close cannot flip a
// bit the open never set. K2's facade 0x0051C760 forwards to an internal
// with K1's pause fields at K1's offsets.
const uintptr_t kAddrSetPauseState = acc::addr::Pick(0x004ae9a0, 0x0051C760);
using PFN_SetPauseState =
    void(__thiscall *)(void* server, int source_bit, unsigned long on_off);
constexpr unsigned char kPauseBitManualOrMenu = 0x02;

// SetSoundMode, mode 0 = playing: unmutes the mixer after a popup close. The
// games differ in signature: K1's 0x005d5e80 is CExoSoundInternal::
// SetSoundMode(mode) on the internal, K2's 0x0070BC60 the CExoSound facade's
// (mode, flag). The ExoSound global holds the matching `this` for each.
const uintptr_t kAddrSetSoundMode = acc::addr::Pick(0x005d5e80, 0x0070BC60);
using PFN_SetSoundMode = void(__thiscall *)(void* self, int mode);
using PFN_SetSoundModeK2 = void(__thiscall *)(void* self, int mode, int flag);
const uintptr_t kAddrExoSoundPtr = acc::addr::PickGlobal(0x007a39ec, 0x00A1B494);

// CClientExoApp::SetPausedByCombat(paused, source, force): despite the name
// the game's real pause, the one the pause key (source 4) and combat
// auto-pause use; it sets the client pause fields that the world update
// checks, plus the sound mode and rumble. The server bit alone does not stop
// the world. K2's facade 0x00740350 (internal 0x0079BF40) writes the same
// three fields at K1's offsets and forks the same way.
const uintptr_t kAddrSetPausedByCombat = acc::addr::Pick(0x005edc20, 0x00740350);
using PFN_SetPausedByCombat =
    void(__thiscall *)(void* clientApp, int paused, int source, int force);

// The live pause byte, accumulated from every OnSetPauseState fire (the
// field Ghidra names pause_state_ reads 0 even while paused).
unsigned char g_pauseShadow = 0;

// True around our own pause calls, so OnSetPauseState stays silent for them;
// the shadow still follows, since the engine state did change.
bool g_inOwnPauseCall = false;

// The mod's overlays that hold the overlay pause (they stack: the world
// resumes only when the last one closes).
unsigned g_overlayPauseOwners = 0;

void CallSetSoundMode(void* exoSound, int mode) {
    if (acc::game::IsKotor2()) {
        reinterpret_cast<PFN_SetSoundModeK2>(kAddrSetSoundMode)(exoSound, mode, 0);
    } else {
        reinterpret_cast<PFN_SetSoundMode>(kAddrSetSoundMode)(exoSound, mode);
    }
}

// The modal-stack size and whether a sub-screen is up; false on a fault.
bool ReadUiState(int* modalSize, bool* hasSubScreen) {
    *modalSize = 0;
    *hasSubScreen = false;
    void* mgr = nullptr;
    if (!TryReadPtr(reinterpret_cast<void*>(kAddrGuiManagerPtr), 0, &mgr)) return false;
    uint32_t size = 0;
    if (mgr && !TryReadU32(mgr, kMgrModalStackSizeOffset, &size)) return false;
    *modalSize = static_cast<int>(size);
    __try {
        *hasSubScreen = HasActiveSubScreen();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    return true;
}

// Unpause after a close: server bit 2 off, then the mixer back on (the
// server round trip does not touch the sound mode). Idempotent.
void DispatchUnpauseCleanup(const char* trigger) {
    acclog::Write("PauseToggle", "%s: SetPauseState(2,0) + SetSoundMode(0)", trigger);
    if (!GetAppManager()) {
        acclog::Write("PauseToggle", "SetPauseState skipped: AppManager NULL");
    } else if (void* server = GetServerApp()) {
        __try {
            g_inOwnPauseCall = true;   // OnSetPauseState fires inside this call
            reinterpret_cast<PFN_SetPauseState>(kAddrSetPauseState)(
                server, kPauseBitManualOrMenu, 0);
            g_inOwnPauseCall = false;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            acclog::Write("PauseToggle", "fault in SetPauseState");
        }
    } else {
        acclog::Write("PauseToggle", "SetPauseState skipped: server NULL");
    }

    void* exoSound = nullptr;
    TryReadPtr(reinterpret_cast<void*>(kAddrExoSoundPtr), 0, &exoSound);
    if (!exoSound) {
        acclog::Write("PauseToggle", "SetSoundMode skipped: ExoSound NULL");
        return;
    }
    __try {
        CallSetSoundMode(exoSound, 0);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write("PauseToggle", "fault in SetSoundMode");
    }
}

// The client app for SetPausedByCombat, or nullptr (logged with `what`). An
// unresolved address is reported as such: a null call under __try would look
// like the engine refusing.
void* PauseClient(const char* what) {
    if (!acc::addr::Ok(kAddrSetPausedByCombat)) {
        acclog::Write("PauseToggle", "%s skipped: SetPausedByCombat unresolved on this "
                      "build — the world will NOT change pause state", what);
        return nullptr;
    }
    if (!GetAppManager()) {
        acclog::Write("PauseToggle", "%s skipped: AppManager NULL", what);
        return nullptr;
    }
    void* client = GetClientApp();
    if (!client) acclog::Write("PauseToggle", "%s skipped: client NULL", what);
    return client;
}

bool CallSetPausedByCombat(void* client, int paused, bool own) {
    __try {
        g_inOwnPauseCall = own;
        reinterpret_cast<PFN_SetPausedByCombat>(kAddrSetPausedByCombat)(client, paused, 4, 0);
        g_inOwnPauseCall = false;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        g_inOwnPauseCall = false;
        return false;
    }
}

// The overlay's own open and close speech is the cue, as native sub-screens
// suppress the engine's, so this call counts as our own.
void DispatchOverlayPause(const char* trigger, int paused) {
    acclog::Write("PauseToggle", "%s: SetPausedByCombat(%d,4,0)", trigger, paused);
    void* client = PauseClient("overlay-pause");
    if (client && !CallSetPausedByCombat(client, paused, /*own=*/true)) {
        acclog::Write("PauseToggle", "fault in overlay-pause SetPausedByCombat");
    }
}

}  // namespace

// The mod's in-world menus have no engine panel, so the engine never pauses
// for them; these give them a native sub-screen's freeze through the pause
// key's own path. Re-asserting on Begin is harmless (the call is idempotent).
void BeginOverlayPause(OverlayPauseOwner owner) {
    g_overlayPauseOwners |= static_cast<unsigned>(owner);
    DispatchOverlayPause("overlay opened", 1);
}

void EndOverlayPause(OverlayPauseOwner owner) {
    g_overlayPauseOwners &= ~static_cast<unsigned>(owner);
    if (g_overlayPauseOwners == 0) {
        DispatchOverlayPause("overlay closed", 0);
    } else {
        acclog::Write("PauseToggle",
                      "overlay closed (owner=0x%02x) — 0x%02x still holds; staying paused",
                      static_cast<unsigned>(owner), g_overlayPauseOwners);
    }
}

bool WorldIsPaused() {
    return (g_pauseShadow & kPauseBitManualOrMenu) != 0;
}

bool ResumeWorldIfPaused(const char* reason) {
    const char* tag = reason ? reason : "?";
    if (!WorldIsPaused()) {
        acclog::Write("PauseToggle", "%s: resume requested but world already running", tag);
        return false;
    }
    // No overlay hold may outlive the resume (this also covers a manual pause
    // with no overlay hold at all).
    g_overlayPauseOwners = 0;
    acclog::Write("PauseToggle", "%s: resume world SetPausedByCombat(0,4,0)", tag);
    void* client = PauseClient("resume");
    if (!client) return false;
    // Not our own call: the engine's "Unpaused" is the menu-close cue here.
    if (!CallSetPausedByCombat(client, 0, /*own=*/false)) {
        acclog::Write("PauseToggle", "fault in ResumeWorldIfPaused");
        return false;
    }
    return true;
}

// The unpause safety net, on two edges (calling both on one frame is safe):
//   1. the modal stack empties: a MessageBox close skips HideSWInGameGui and
//      leaves the pause bit set and the sound muted. Not while a sub-screen
//      is still up (a tutorial or confirm over a panel): the panel's own
//      close unpauses later.
//   2. the last sub-screen leaves panels[]: Options -> Save -> Esc -> Esc
//      can leave the engine's own close half done. Closing one sub-screen
//      while another stays open is no edge.
void TickInputClassReassert() {
    static int  s_prevModalSize    = 0;
    static bool s_prevHasSubScreen = false;

    int  modalSize = 0;
    bool hasSubScreen = false;
    if (!ReadUiState(&modalSize, &hasSubScreen)) return;

    if (s_prevModalSize > 0 && modalSize == 0) {
        if (!hasSubScreen) {
            char tag[64];
            snprintf(tag, sizeof(tag), "popup closed (modal_stack %d->0)", s_prevModalSize);
            DispatchUnpauseCleanup(tag);
        } else {
            acclog::Write("PauseToggle", "popup closed (modal_stack %d->0): SUPPRESSED — "
                          "sub-screen still in panels[]", s_prevModalSize);
        }
    }
    if (s_prevHasSubScreen && !hasSubScreen) {
        DispatchUnpauseCleanup("last sub-screen left panels[]");
    }
    s_prevModalSize    = modalSize;
    s_prevHasSubScreen = hasSubScreen;
}

}  // namespace acc::engine

namespace {

// The hooks' parameters are the addresses of the engine's stack slots (the
// "esp+X" LEA); the return address sits one slot below the first.
bool ReadSlots(void* p1_addr, void* p2_addr, int* p1, int* p2, uint32_t* callerEip) {
    __try {
        *p1 = *reinterpret_cast<int*>(p1_addr);
        if (p2_addr) *p2 = *reinterpret_cast<int*>(p2_addr);
        *callerEip = *(reinterpret_cast<uint32_t*>(p1_addr) - 1);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

}  // namespace

// CGuiInGame::SwitchToSWInGameGui, the warm path (a sub-screen is up and
// another opens; the cold path is ShowSWInGameGui, which the menus monitor
// covers by polling). The prior sub-screen is popped first, so the engine's
// push lands on a clean panels[], the cascade the engine's own close paths
// use.
extern "C" void __cdecl OnSwitchToSWInGameGui(void* thisPtr, int guiId) {
    if (!acc::engine::g_switchHookEverFired) {
        acc::engine::g_switchHookEverFired = true;   // proves the hook is alive
        acclog::Write("SubScreen.Switch", "first fire: this=%p GUI_id=%d (hook is alive)",
                      thisPtr, guiId);
    }
    if (!acc::engine::HasActiveSubScreen()) return;
    acclog::Write("SubScreen.Switch", "redrill cleanup: this=%p new GUI_id=%d (closing prior "
                  "sub-screen via PrevSWInGameGui)", thisPtr, guiId);
    acc::engine::CallPrevSWInGameGui();
}

// CGuiInGame::SetSWGuiStatus. Status 4 means a sub-screen is finishing: its
// controls are about to be freed, so the cached chain and the pending
// announce (a SetActive just before an area load) are dropped now.
extern "C" void __cdecl OnSetSWGuiStatus(void* thisPtr, void* p1_addr, void* p2_addr) {
    if (!p1_addr || !p2_addr) return;
    int newStatus = -1, p2 = -1;
    uint32_t callerEip = 0;
    if (!ReadSlots(p1_addr, p2_addr, &newStatus, &p2, &callerEip)) {
        acclog::Write("SubScreen.Status", "deref faulted (this=%p p1=%p p2=%p)",
                      thisPtr, p1_addr, p2_addr);
        return;
    }
    // Trace: the engine re-asserts the same status every frame while a
    // sub-screen is up; the dedup keeps the count.
    acclog::Trace("SubScreen.Status", "this=%p new_status=%d p2=%d caller=0x%08x",
                  thisPtr, newStatus, p2, callerEip);
    if (newStatus == 4) {
        acc::menus::chain::InvalidateChain();
        acc::menus::ClearPendingAnnounce();
    }
}

// CGuiInGame::HideSWInGameGui: which engine path closes a sub-screen.
extern "C" void __cdecl OnHideSWInGameGui(void* thisPtr, void* p1_addr) {
    if (!p1_addr) return;
    int p1 = -1, unused = 0;
    uint32_t callerEip = 0;
    if (!ReadSlots(p1_addr, nullptr, &p1, &unused, &callerEip)) {
        acclog::Write("SubScreen.Hide", "deref faulted (this=%p p1=%p)", thisPtr, p1_addr);
        return;
    }
    acclog::Write("SubScreen.Hide", "this=%p param_1=%d caller=0x%08x", thisPtr, p1, callerEip);
}

// CServerExoAppInternal::SetPauseState (K1 0x004b8110): every pause change.
// Speaks "Paused" / "Resumed" on a bit-2 flip that is not our own and comes
// with no popup or sub-screen up (those have their own speech).
extern "C" void __cdecl OnSetPauseState(void* thisPtr, void* p1_addr, void* p2_addr) {
    using acc::engine::g_pauseShadow;
    using acc::engine::kPauseBitManualOrMenu;
    if (!p1_addr || !p2_addr) return;
    int mask = 0, onOff = 0;
    uint32_t callerEip = 0;
    if (!ReadSlots(p1_addr, p2_addr, &mask, &onOff, &callerEip)) {
        acclog::Write("Pause", "deref faulted (this=%p)", thisPtr);
        return;
    }

    const unsigned char prev = g_pauseShadow;
    const unsigned char maskByte = static_cast<unsigned char>(mask);
    const unsigned char next = onOff ? static_cast<unsigned char>(prev | maskByte)
                                     : static_cast<unsigned char>(prev & ~maskByte);
    g_pauseShadow = next;
    acclog::Write("Pause", "fire mask=0x%02x on_off=%d shadow %02x->%02x caller=0x%08x%s",
                  static_cast<unsigned>(maskByte), onOff, static_cast<unsigned>(prev),
                  static_cast<unsigned>(next), callerEip,
                  acc::engine::g_inOwnPauseCall ? " (SELF)" : "");

    if (acc::engine::g_inOwnPauseCall) return;
    if ((maskByte & kPauseBitManualOrMenu) == 0) return;
    const bool wasPaused = (prev & kPauseBitManualOrMenu) != 0;
    const bool nowPaused = (next & kPauseBitManualOrMenu) != 0;
    if (wasPaused == nowPaused) return;

    int modalSize = 0;
    bool hasSubScreen = false;
    if (!acc::engine::ReadUiState(&modalSize, &hasSubScreen)) return;
    if (modalSize > 0 || hasSubScreen) {
        acclog::Write("Pause", "speech SUPPRESSED (modal=%d subscreen=%d)",
                      modalSize, hasSubScreen ? 1 : 0);
        return;
    }
    acc::speech::Speak(acc::strings::Get(nowPaused ? acc::strings::Id::GamePaused
                                                   : acc::strings::Id::GameResumed),
                       /*interrupt=*/false);
}

// KOTOR 2 frame wrappers. K2 compiles its GUI unoptimised, so the hook passes
// ECX (`this`) and EBP; [EBP+8] is the first argument's slot and [EBP+4] the
// return address, exactly the slot addresses the K1 handlers expect.

// CGuiInGame::SwitchToSWInGameGui @0x007CA550, hooked at 0x007CA575.
extern "C" void __cdecl OnSwitchToSWInGameGuiK2(void* thisGui, void* ebp) {
    if (!acc::game::IsKotor2() || !thisGui || !ebp) return;
    uint32_t guiId = 0;
    if (!acc::engine::TryReadU32(ebp, 8, &guiId)) return;
    OnSwitchToSWInGameGui(thisGui, static_cast<int>(guiId));
}

// CGuiInGame::HideSWInGameGui @0x007CA060, hooked at 0x007CA066.
extern "C" void __cdecl OnHideSWInGameGuiK2(void* thisGui, void* ebp) {
    if (!acc::game::IsKotor2() || !thisGui || !ebp) return;
    OnHideSWInGameGui(thisGui, static_cast<char*>(ebp) + 8);
}

// CGuiInGame::SetSWGuiStatus @0x007C9C40, hooked at 0x007C9C46.
extern "C" void __cdecl OnSetSWGuiStatusK2(void* thisGui, void* ebp) {
    if (!acc::game::IsKotor2() || !thisGui || !ebp) return;
    OnSetSWGuiStatus(thisGui, static_cast<char*>(ebp) + 8, static_cast<char*>(ebp) + 0xC);
}

// CServerExoAppInternal::SetPauseState @0x00538ED0, hooked at 0x00538ED9 (K1's
// pause fields at the same offsets).
extern "C" __declspec(dllexport)
void __cdecl OnSetPauseStateK2(void* thisPtr, void* ebp) {
    if (!acc::game::IsKotor2() || !thisPtr || !ebp) return;
    OnSetPauseState(thisPtr, static_cast<char*>(ebp) + 8, static_cast<char*>(ebp) + 0xC);
}

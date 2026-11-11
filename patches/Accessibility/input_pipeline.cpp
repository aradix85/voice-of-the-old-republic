// The engine's in-world input route, CClientExoAppInternal::HandleInputEvent
// (both games): which presses the mod takes before the engine sees them, the
// preparation of the bare action keys 1..7, and the frame sequence counter.

#include "input_pipeline.h"

#include "combat_diag.h"
#include "combat_queue.h"
#include "engine_actionbar.h"
#include "engine_area.h"
#include "engine_game.h"
#include "engine_input.h"
#include "engine_keymap.h"
#include "engine_manager.h"
#include "engine_panels.h"
#include "engine_player.h"
#include "engine_radial.h"
#include "engine_reads.h"
#include "engine_addr.h"
#include "examine_view.h"
#include "floor_puzzle.h"
#include "help.h"
#include "hotkeys.h"
#include "log.h"
#include "minigame_swoop_race.h"
#include "narrated_target.h"
#include "passive_narrate.h"
#include "qe_ring.h"
#include "unified_action_menu.h"

#include <windows.h>

#include <cstdint>

namespace acc::input {

namespace {

volatile LONG s_seq = 0;   // wraps after 4G events; only adjacency matters

// The overlay-Esc and editbox-submit latches: GetTickCount() of the poll-side
// close, 0 when unset. The poll and the engine event of one press land in the
// same or the next frame, so the window is short.
DWORD s_escClosedAt = 0;
constexpr DWORD kEscLatchWindowMs = 150;
DWORD s_editboxSubmitAt = 0;
constexpr DWORD kEditboxSubmitLatchWindowMs = 150;

bool ConsumeLatch(DWORD& at, DWORD window) {
    if (at == 0) return false;
    const DWORD elapsed = GetTickCount() - at;  // wrap-safe
    at = 0;                                     // single-shot
    return elapsed <= window;
}

}  // namespace

unsigned int NextSeq() {
    return static_cast<unsigned int>(InterlockedIncrement(&s_seq));
}

void NoteOverlayEscClosed() {
    s_escClosedAt = GetTickCount();
}

bool ConsumeOverlayEscLatch() {
    return ConsumeLatch(s_escClosedAt, kEscLatchWindowMs);
}

void NoteEditboxSubmitClosed() {
    s_editboxSubmitAt = GetTickCount();
}

bool ConsumeEditboxSubmitLatch() {
    return ConsumeLatch(s_editboxSubmitAt, kEditboxSubmitLatchWindowMs);
}

namespace {

constexpr uint32_t kReservedMods =
    acc::hotkeys::kModShift | acc::hotkeys::kModCtrl | acc::hotkeys::kModAlt;

struct InputEvent {
    int          code;
    int          value;      // 0 on release
    uint32_t     callerEip;
    unsigned int seq;
};

// The two event args sit in the engine's frame at the addresses the hook
// passes (LEA of the slots); the return address is one slot below.
bool ReadEvent(void* p1_addr, void* p2_addr, InputEvent* ev) {
    __try {
        ev->code      = *reinterpret_cast<int*>(p1_addr);
        ev->value     = *reinterpret_cast<int*>(p2_addr);
        ev->callerEip = *(reinterpret_cast<uint32_t*>(p1_addr) - 1);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void LogEvent(const InputEvent& ev, void* this_ptr) {
    const int translated = acc::engine::ManagerTranslateCode(ev.code);
    if (translated != ev.code) {
        acclog::Write("Diag.ClientHIE",
                      "seq=%u this=%p caller=0x%08x key=logical(%d) -> %s(%d) val=%d",
                      ev.seq, this_ptr, ev.callerEip, ev.code,
                      acc::engine::InputIndexName(translated), translated, ev.value);
    } else {
        acclog::Write("Diag.ClientHIE",
                      "seq=%u this=%p caller=0x%08x key=%s(%d) val=%d",
                      ev.seq, this_ptr, ev.callerEip,
                      acc::engine::InputIndexName(ev.code), ev.code, ev.value);
    }
}

bool IsGuiNavForward(int code) {
    return code >= 0xb6 && code <= 0xb9;
}

// The engine's logical codes for bare 1..7: 1..3 are DoTargetAction rows 0..2;
// 4..7 are DoPersonalAction columns 0..3, linear in the key number, though the
// codes of keys 6 and 7 are swapped (6 = 0xee, 7 = 0xec).
int TargetRowForCode(int code) {
    switch (code) {
        case 0xe2: return 0;
        case 0xe4: return 1;
        case 0xe6: return 2;
        default:   return -1;
    }
}

int PersonalColumnForCode(int code) {
    switch (code) {
        case 0xe8: return 0;   // key 4, Friendly Force
        case 0xea: return 1;   // key 5, Medical
        case 0xee: return 2;   // key 6, Miscellaneous
        case 0xec: return 3;   // key 7, Explosives
        default:   return -1;
    }
}

bool IsBareActionCode(int code) {
    return TargetRowForCode(code) >= 0 || PersonalColumnForCode(code) >= 0;
}

const char* BareLabel(int code) {
    static const char* const kLabels[] = {
        "bare-1", "bare-2", "bare-3", "bare-4", "bare-5", "bare-6", "bare-7",
    };
    const int row = TargetRowForCode(code);
    if (row >= 0) return kLabels[row];
    const int col = PersonalColumnForCode(code);
    return col >= 0 ? kLabels[3 + col] : "bare-?";
}

// SetTarget + RePopulate of the engine's action lists against the narrated
// target (kInvalidObjectId when none resolves: empty target rows rather than
// stale ones). Only from HandleInputEvent: repopulating from the poll/tick
// context synthesised a phantom confirm.
void PrepareBareDispatchForNarratedTarget() {
    uint32_t targetClient = kInvalidObjectId;
    acc::narrated_target::Slot slot{};
    if (acc::narrated_target::TryGet(slot) && !slot.isMapPin &&
        slot.handle != 0u && slot.handle != kInvalidObjectId &&
        acc::engine::ResolveServerObjectHandle(slot.handle)) {
        targetClient = slot.handle | 0x80000000u;   // the engine wants client handles
    }
    (void)acc::engine_actionbar::PrepareBareDispatch(targetClient);
}

// PopulateMenus hands out fresh action ids, so the user's chosen variant is
// stamped again after the repopulate (DoPersonalAction falls back to variant
// 0 when the selected id matches nothing). The column layout is the engine's,
// not that of our labels.
void RestampPersonalColumn(int col) {
    void* mi = acc::engine_actionbar::ResolveMainInterface();
    if (!mi) return;
    const int idx = acc::unified_menu::PersonalSelection(col);
    (void)acc::engine_actionbar::SelectVariant(mi, col, idx);
    acc::engine_actionbar::LogDispatchDiag(mi, col, idx);
    // A K2 medical entry cannot fire through the engine's dispatch (its
    // preamble wipes the two-step pick, see SendUseItemRequestK2), so the
    // use-item request goes straight at the controlled leader: instant
    // self-use. The engine's dispatch after us only re-arms its pick.
    if (!acc::engine_actionbar::EntryIsMedicalK2(mi, col, idx)) return;
    const uint32_t aid = acc::engine_actionbar::ReadVariantActionId(mi, col, idx);
    void* leader = acc::engine::GetClientLeader();
    if (aid != 0 && leader) {
        const bool sent = acc::engine_actionbar::SendUseItemRequestK2(aid, leader);
        acclog::Write("ActionBar", "bare medical direct send id=0x%08x sent=%d",
                      aid, sent ? 1 : 0);
    }
}

// The same for the target rows: DoTargetAction searches the row's list for
// the stamped id, falling back to the first entry.
void RestampTargetRow(int row) {
    void* tam = acc::engine_radial::ResolveTargetActionMenu();
    if (!tam) return;
    const int idx = acc::unified_menu::TargetSelection(row);
    (void)acc::engine_radial::SelectActionInRow(tam, row, idx);
}

// KOTOR 2 swoop race: Space (event 0xd9) is the engine's jump, and the mod's
// runner takes it so every jump is logged beside the race state. No-op
// outside a K2 race; on KOTOR 1 Space is the shift and stays the engine's.
bool TrySwoopJump(const InputEvent& ev) {
    if (!acc::game::IsKotor2() || ev.value == 0 || ev.code != 0xd9) return false;
    if (!acc::swoop_race::TryJump()) return false;
    acclog::Write("Diag.ClientHIE", "seq=%u code=0x%x CONSUMED — K2 swoop jump",
                  ev.seq, ev.code);
    return true;
}

// In the Rakatan floor-plate room bare R reads the board (floor_puzzle), so
// the engine's DefaultAction on the same press is swallowed. Modified R goes
// on to the modifier reservation.
bool TryPuzzleBareR(const InputEvent& ev) {
    if (ev.value == 0 || ev.code != kInputDefaultAction || !acc::floor_puzzle::IsActive()) {
        return false;
    }
    if ((acc::hotkeys::CurrentModifiers() & kReservedMods) != 0) return false;
    acclog::Write("Diag.ClientHIE", "seq=%u DefaultAction(R) CONSUMED — puzzle room owns "
                  "bare R (board readout)", ev.seq);
    return true;
}

// The engine reads modifier-blind scancodes and has no combos of its own:
// Shift+4 still fires bare 4. So in-world every modified press is the mod's
// (a hotkey, or kept free for one) and the engine never sees it; bare keys
// stay the engine's. Movement keys never arrive here; Esc and the GUI arrow
// forward keep their own routes below. Shift+1..7 open the unified menu from
// the poller this frame, which reads the lists without repopulating, so they
// are rebuilt here as for a bare press.
bool TryModifierReservation(const InputEvent& ev) {
    if (ev.value == 0) return false;
    const uint32_t m = acc::hotkeys::CurrentModifiers();
    if ((m & kReservedMods) == 0) return false;
    if (ev.code == kInputEsc1 || ev.code == kInputEsc2 || IsGuiNavForward(ev.code)) return false;

    int vks[4];
    const int nv = acc::engine_keymap::VksForCode(ev.code, vks, 4);
    bool owned = false;
    for (int i = 0; i < nv && !owned; ++i) owned = acc::hotkeys::ModifiedComboOwns(vks[i]);
    acclog::Write("Diag.ModShadow", "seq=%u code=%s(%d) mods=0x%x CONSUMED — %s",
        ev.seq, acc::engine::InputIndexName(ev.code), ev.code, m,
        owned ? "mod hotkey owns this combo; engine action suppressed"
              : "modifier space reserved (no mod binding yet); engine action suppressed");
    if (IsBareActionCode(ev.code) && !acc::engine::HasActiveDialogPanel()) {
        PrepareBareDispatchForNarratedTarget();
    }
    return true;
}

// The mod's in-world overlays (action menus, queue, examine, help) have no
// engine panel, so Esc lands here, where the engine would open the pause menu
// while the overlay's own poll closes it. Taken while an overlay is open, or
// was just closed by this same press (the latch). Not while the unified menu
// is suspended under an engine panel: Esc then dismisses that panel.
bool TryOverlayEsc(const InputEvent& ev) {
    if (ev.value == 0 || (ev.code != kInputEsc1 && ev.code != kInputEsc2)) return false;
    const bool overlayActive =
        (acc::unified_menu::IsActive() && !acc::unified_menu::IsSuspended()) ||
        acc::combat::queue::IsActive() || acc::examine_view::IsActive() ||
        acc::help::IsMenuOpen();
    // The live overlay first, so the latch is not spent on an Esc not ours.
    if (!overlayActive && !ConsumeOverlayEscLatch()) return false;
    acclog::Write("Diag.ClientHIE", "seq=%u Esc CONSUMED — in-world overlay %s "
                  "(suppressing engine pause-menu open)", ev.seq,
                  overlayActive ? "active" : "just-closed (latch)");
    return true;
}

// Bare 1..7: the engine rebuilds its action lists only on a combat-mode flip,
// a sub-screen close or mouse hover, not on a Q/E target change, so between
// rounds they hold a stale creature_id (grenades at friends). Rebuild them
// against the narrated target before the engine's switch runs, and stamp the
// chosen variants again. Not while a dialog's reply list owns the number keys:
// the refresh would make the combat side fire as well.
void PrepareBareActionKey(const InputEvent& ev) {
    if (ev.value == 0 || !IsBareActionCode(ev.code) || acc::engine::HasActiveDialogPanel()) {
        return;
    }
    // The queue depth before the engine's AddAction (which drops a fifth
    // action), the press attribution for the AddAction detour's "X, position
    // N", and a no-op watch on the personal columns (target rows act without
    // a combat-round add).
    acc::combat::queue::ReportPrePressDepth();
    acc::combat::queue::ArmUserQueueAdd();
    const int col = PersonalColumnForCode(ev.code);
    if (col >= 0) acc::combat::queue::ArmNoOpWatch();
    acc::combat_diag::LogPreFire(BareLabel(ev.code));

    PrepareBareDispatchForNarratedTarget();
    if (col >= 0) RestampPersonalColumn(col);
    const int row = TargetRowForCode(ev.code);
    if (row >= 0) RestampTargetRow(row);
}

// Q (205) / E (204). With "Q and E: everything around you" the mod cycles all
// objects around and points the engine's target and lists at the landing.
// Otherwise the re-announce for the single-enemy case (where the engine's
// cycle changes nothing) waits a frame, so it never speaks the old target
// before the engine's ShowObject speaks the new one.
bool TryQE(const InputEvent& ev) {
    if (ev.value == 0 || (ev.code != 204 && ev.code != 205)) return false;
    if (acc::passive_narrate::IsInSynthesizedQE()) return false;  // our own re-issue
    if (acc::qe_ring::HandlePress(ev.code == 204 ? +1 : -1)) {
        PrepareBareDispatchForNarratedTarget();
        acclog::Write("Diag.ClientHIE", "seq=%u Q/E CONSUMED — all-around cycle", ev.seq);
        return true;
    }
    acc::passive_narrate::RequestQEReannounce(ev.code);
    return false;
}

// A MessageBox goes on the modal stack without changing sw_gui_status, so the
// arrow codes are routed here, where nothing handles them, instead of to the
// manager: the user could not move between the popup's buttons. Forward them
// to CSWGuiManager::HandleInputEvent (same code space in both games).
void ForwardModalArrows(const InputEvent& ev) {
    if (ev.value == 0 || !IsGuiNavForward(ev.code)) return;
    void* mgr = nullptr;
    uint32_t modalSize = 0;
    if (!acc::engine::TryReadPtr(reinterpret_cast<void*>(kAddrGuiManagerPtr), 0, &mgr) ||
        !mgr || !acc::engine::TryReadU32(mgr, kMgrModalStackSizeOffset, &modalSize) ||
        static_cast<int>(modalSize) <= 0) {
        return;
    }
    using PFN_MgrHIE = void(__thiscall*)(void*, int, int);
    const uintptr_t kAddrMgrHIE = acc::addr::Pick(0x0040c8e0, 0x00410AA0);
    __try {
        reinterpret_cast<PFN_MgrHIE>(kAddrMgrHIE)(mgr, ev.code, ev.value);
        acclog::Write("Diag.ClientHIE", "seq=%u arrow forwarded to manager (modal_stack=%d)",
                      ev.seq, static_cast<int>(modalSize));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write("Diag.ClientHIE", "seq=%u arrow-forward fault (mgr=%p)", ev.seq, mgr);
    }
}

// 1 consumes the event (the hook's consumed exit), 0 lets the engine run.
int HandleClientInputEvent(void* this_ptr, void* p1_addr, void* p2_addr) {
    if (!p1_addr || !p2_addr) return 0;
    InputEvent ev{};
    if (!ReadEvent(p1_addr, p2_addr, &ev)) {
        acclog::Write("Diag.ClientHIE", "deref faulted (this=%p p1=%p p2=%p)",
                      this_ptr, p1_addr, p2_addr);
        return 0;
    }
    ev.seq = NextSeq();
    // 128 is a DirectInput key press (codes 65/66 are the mouse axes, with
    // pixel values): the keyboard-liveness stamp.
    if (ev.value == 128) acc::engine::NoteEngineKeyEvent();
    LogEvent(ev, this_ptr);

    if (TrySwoopJump(ev) || TryPuzzleBareR(ev) || TryModifierReservation(ev) ||
        TryOverlayEsc(ev)) {
        return 1;
    }
    PrepareBareActionKey(ev);
    if (TryQE(ev)) return 1;
    ForwardModalArrows(ev);
    return 0;
}

}  // namespace

}  // namespace acc::input

// CClientExoAppInternal::ProcessInput (K1 hook 0x006227fb, K2 0x007B0EAB):
// the frame boundary, kept as a gap in the sequence of the input logs.
extern "C" void __cdecl OnProcessInput(void* /*this_ptr*/) {
    acc::input::NextSeq();
}

// CClientExoAppInternal::HandleInputEvent, hooked at its entry (K1 0x00621210):
// every event ProcessInput sends the client-app way (in-world hotkeys, pause,
// party cycle, target actions, Esc). The parameters are the addresses of the
// two stack slots (KPatchManager's LEA for "esp+X" sources).
extern "C" int __cdecl OnClientHandleInputEvent(void* this_ptr, void* p1_addr,
                                                void* p2_addr) {
    return acc::input::HandleClientInputEvent(this_ptr, p1_addr, p2_addr);
}

// KOTOR 2 (0x007B12C0, hooked at 0x007B12EE): ecx = this, ebp = the engine's
// frame with the args at [ebp+8]/[ebp+0xc] and the return address at [ebp+4],
// the same slot layout as K1, so the shared handler runs unchanged.
extern "C" int __cdecl OnClientHandleInputEventK2(void* this_ptr, void* frame_ebp) {
    if (!frame_ebp) return 0;
    unsigned char* frame = static_cast<unsigned char*>(frame_ebp);
    return acc::input::HandleClientInputEvent(this_ptr, frame + 8, frame + 12);
}

// GUI input dispatch and the focus-change hook. Two engine hooks:
//
//   * OnHandleFocusChange (CSWGuiControl::HandleFocusChange): log only.
//   * OnHandleInputEvent (CSWGuiManager::HandleInputEvent): the ordered gate
//     list every GUI key event runs through.
//
// Gate ORDER is behaviour: each subsystem gets first refusal in a fixed
// sequence, so nothing here may be reordered without an in-game input pass.

#include <windows.h>
#include <cstdint>

#include "bringup_announce.h"
#include "cycle_input.h"
#include "engine_input.h"
#include "engine_keymap.h"
#include "engine_manager.h"
#include "engine_offsets.h"
#include "engine_panels.h"
#include "engine_reads.h"
#include "help.h"
#include "hotkeys.h"
#include "input_pipeline.h"
#include "log.h"
#include "menus.h"
#include "menus_abilities.h"
#include "menus_chain.h"
#include "menus_chargen_feats.h"
#include "menus_editbox.h"
#include "menus_galaxymap.h"
#include "menus_internal.h"
#include "menus_keymap.h"
#include "menus_listbox.h"
#include "menus_modsettings.h"
#include "menus_monitors.h"
#include "menus_pazaakdeck.h"
#include "menus_powers_levelup.h"
#include "minigame_pazaak.h"
#include "peek_description.h"
#include "speech.h"

using namespace acc::engine;

using acc::menus::chain::g_chain;
using acc::menus::chain::g_chainPanel;
using acc::menus::chain::g_chainIndex;
using acc::menus::chain::g_chainCount;

namespace {

// One manager input event. val: 0 = release, 128 = a DirectInput press,
// 1 = a press the engine synthesised itself.
struct Event {
    int          n;      // call counter, for the log
    unsigned int seq;    // shared with Diag.ClientHIE lines (input_pipeline.h)
    void*        mgr;
    int          code;   // InputIndices key or button code
    int          val;
};

bool IsPress(const Event& e) { return e.val != 0; }
bool IsEnter(int code) { return code == kInputEnter1 || code == kInputEnter2; }
bool IsEsc(int code) { return code == kInputEsc1 || code == kInputEsc2; }

void LogEvent(const Event& e, const char* tag) {
    const int translated = ManagerTranslateCode(e.code);
    if (translated != e.code) {
        acclog::Write("Menus.Input", "#%d seq=%u this=%p key=logical(%d) -> %s(%d) val=%d%s",
                      e.n, e.seq, e.mgr, e.code, InputIndexName(translated), translated,
                      e.val, tag);
    } else {
        acclog::Write("Menus.Input", "#%d seq=%u this=%p key=%s(%d) val=%d%s",
                      e.n, e.seq, e.mgr, InputIndexName(e.code), e.code, e.val, tag);
    }
}

// Press-release pairing. When a handler here consumes a press (Enter on a
// chain entry queues an activate, say), the engine never sees it, but the
// matching RELEASE still arrives and the engine's release path fires onClick
// on whatever is focused by then: Enter on "Load Game" opened the save list
// and the release loaded the first save. So the code of a consumed press is
// kept and its release consumed too. Only for presses consumed here: the
// engine needs the releases of the presses it did see. One slot is enough,
// the engine sends press then release per key.
int s_lastConsumedPress = 0;

// Every return of a press goes through here with its verdict (1 = consumed).
int TrackPress(const Event& e, int rv) {
    // The first consume proves the engine's input pump is live and routing
    // to the GUI manager (during the starved bring-up window this hook does
    // not fire at all), which ends the "still loading" nag. Idempotent.
    if (rv == 1) acc::bringup_announce::NotifyInputPumpLive();
    // A press synthesised by PollHomeEndKeys has no release coming, and must
    // not clear the slot of a real press still waiting for its release.
    if (IsPress(e) && !acc::menus::s_synthesizedNav) {
        s_lastConsumedPress = (rv == 1) ? e.code : 0;
    }
    return rv;
}

bool IsReleaseOfConsumedPress(const Event& e) {
    if (IsPress(e) || s_lastConsumedPress == 0 || s_lastConsumedPress != e.code) {
        return false;
    }
    LogEvent(e, " PAIR-CONSUMED (matches consumed press)");
    s_lastConsumedPress = 0;
    return true;
}

// The engine is modifier-blind: a mod hotkey that reuses an engine GUI key
// with a modifier would also drive the engine's bare-key handling. True when
// a registered mod binding owns the combo on this code's physical key (the
// manager-side twin of the in-world consume in input_pipeline.cpp).
bool IsModifierShadowed(const Event& e) {
    int vks[4];
    const int count = acc::engine_keymap::VksForCode(e.code, vks, 4);
    for (int i = 0; i < count; ++i) {
        if (!acc::hotkeys::ModifiedComboOwns(vks[i])) continue;
        acclog::Write("Menus.Input",
                      "#%d seq=%u this=%p key=%s(%d) CONSUMED - modifier-shadowed "
                      "mod hotkey owns vk=0x%02x (mods=0x%x)",
                      e.n, e.seq, e.mgr, InputIndexName(e.code), e.code, vks[i],
                      acc::hotkeys::CurrentModifiers());
        return true;
    }
    return false;
}

// A physical F1 reaches the manager as kInputActivate (0x27), the engine's
// GUI "activate" code, and would fire the focused control; help::PollWin32
// owns F1. While the help list is open its own Win32 polling navigates it,
// and the panel beneath must not see the nav, Enter and Esc events.
bool HelpOwns(const Event& e) {
    if (e.code == kInputActivate) {
        acclog::Write("Menus.Input", "#%d seq=%u this=%p key=F1(0x27 activate-code) "
                      "HELP-F1-SUPPRESSED", e.n, e.seq, e.mgr);
        return true;
    }
    if (!acc::help::IsMenuOpen()) return false;
    switch (e.code) {
    case kInputNavUp:   case kInputNavDown:
    case kInputNavLeft: case kInputNavRight:
    case kInputHome:    case kInputEnd:
    case kInputEnter1:  case kInputEnter2:
    case kInputEsc1:    case kInputEsc2:
        LogEvent(e, " HELP-CONSUMED");
        return true;
    default:
        return false;
    }
}

// The panel the event is for: the manager's foreground (modal_stack or
// panels[]). g_currentPanel, the last panel that got SetActiveControl, is
// unreliable for routing: flows that create several panels in one frame
// (character creation: a modal and two wizards) leave it on a backgrounded
// one. It is only the fallback for frames without a foreground, and then
// liveness-filtered, because a module load can leave it on a freed panel.
void* ResolveActivePanel() {
    void* mgr = *reinterpret_cast<void**>(kAddrGuiManagerPtr);
    void* fg = GetForegroundPanel(mgr);
    void* activePanel = fg ? fg : CurrentPanelIfLive();
    if (fg && fg != g_currentPanel) {
        // Once per (fg, current) pair.
        static void* s_lastFg = nullptr;
        static void* s_lastCp = nullptr;
        if (fg != s_lastFg || g_currentPanel != s_lastCp) {
            acclog::Write("Routing", "fg=%p current=%p (using fg)", fg, g_currentPanel);
            s_lastFg = fg;
            s_lastCp = g_currentPanel;
        }
    }
    // Drilled into a sub-screen: retarget from the in-game menu strip (kept
    // in front by SendPanelToBack) to the sub-screen panel. Only when the
    // foreground IS the strip; tutorial modals and Options sub-tabs are the
    // foreground themselves.
    if (g_drilledIntoSubScreen && IdentifyPanel(activePanel) == PanelKind::InGameMenu) {
        if (void* sub = acc::menus::monitors::FindActiveSubScreenPanel()) {
            activePanel = sub;
        } else {
            g_drilledIntoSubScreen = false;
            acclog::Write("Drill", "sub-screen gone from panels[]; returning to strip");
        }
    }
    return activePanel;
}

// The chain's focused control when the chain is on this panel. The peek
// handlers need it: panel.activeControl tracks a helper control during
// chain navigation, not the item row.
void* ChainFocusOn(void* panel) {
    if (g_chainPanel != panel || g_chainIndex < 0 || g_chainIndex >= g_chainCount) {
        return nullptr;
    }
    return g_chain[g_chainIndex].control;
}

}  // namespace

// CSWGuiControl::HandleFocusChange, hooked mid-function at 0x41896b. Log
// only: the panel-level SetActiveControl hook is the announcement signal,
// and this one fires twice per navigation (old loses, new gains focus).
extern "C" void __cdecl OnHandleFocusChange(void* thisPtr, int param_1) {
    EnsureSpeechInitialized();
    static int n = 0;
    ++n;
    const char* tip; uint32_t tipLen; int id;
    ReadControlNameFields(thisPtr, tip, tipLen, id);
    acclog::Write("Menus.FocusChange", "#%d this=%p p1=%d id=%d tip[%u]=\"%s\"",
                  n, thisPtr, param_1, id, tipLen,
                  (tip && tipLen > 0) ? tip : "");
}

// CSWGuiManager::HandleInputEvent, hooked mid-function at 0x0040c907: the
// GUI manager's central dispatcher, which every key or mouse event for any
// GUI surface passes before it goes to the active panel's own handler. The
// cut sits BEFORE the param_2 == 0 early-out, so releases arrive too. At
// the cut: ECX = this, EBX = param_1 (code), EAX = param_2 (state).
// Returns 1 when the event is consumed.
//
// Both games: the generic path uses only constants verified for both;
// panel handlers whose KOTOR 2 constants are missing decline at their own
// entry and the key falls through to the generic chain.
extern "C" int __cdecl OnHandleInputEvent(void* thisPtr, int param_1, int param_2) {
    EnsureSpeechInitialized();
    static int n = 0;
    const Event e{++n, acc::input::NextSeq(), thisPtr, param_1, param_2};

    if (IsReleaseOfConsumedPress(e)) return 1;
    if (IsPress(e) && IsModifierShadowed(e)) return TrackPress(e, 1);

    // An Esc the engine synthesised itself (val 1; DirectInput presses carry
    // 128): CClientExoAppInternal::HandleInputEvent reissues Esc to the
    // manager when its in-world handling cannot run, typically under a
    // message box. Pass it through: the modal's own Esc handler closes it
    // AND runs the engine's cleanup (input class, mouse, gui status), which
    // a queued cancel-activate from here did not ("walking and Enter break"
    // after an Esc-dismiss). Not tracked: nothing was consumed.
    if (e.val == 1 && IsEsc(e.code)) {
        acclog::Write("Menus.Input", "#%d seq=%u this=%p key=logical(%d) val=1 "
                      "SYNTHESISED-PASSTHROUGH (upstream case 0xdf reissue)",
                      e.n, e.seq, e.mgr, e.code);
        return 0;
    }

    // The mod-settings submenu has no engine panel of its own: while it is
    // open the Options panel is still the foreground and would take the keys.
    if (IsPress(e) && acc::menus::modsettings::IsOpen() &&
        acc::menus::modsettings::HandleInput(e.code)) {
        LogEvent(e, " MOD-SETTINGS-CONSUMED");
        return TrackPress(e, 1);
    }
    if (IsPress(e) && HelpOwns(e)) return TrackPress(e, 1);

    // An Enter that reaches the manager belongs to the GUI. Without the
    // claim, the Enter that ends a dialog would also fire the world Interact
    // on the still-narrated target at the next tick. ClaimRisingEdge, not
    // Consume: the engine dispatches manager input BETWEEN EndTick and the
    // next BeginTick, where Consume has no effect; the claim survives
    // BeginTick.
    if (IsPress(e) && IsEnter(e.code)) {
        acc::hotkeys::ClaimRisingEdge(acc::hotkeys::Action::InteractTarget);
        acc::hotkeys::ClaimRisingEdge(acc::hotkeys::Action::InteractForceRadial);
    }

    void* panel = ResolveActivePanel();

    // The in-world action menu (Shift+Enter, Shift+1..7) is not routed here:
    // it takes its keys through input_poll_router's Win32 poll.

    // Panel handlers, each with first refusal over those below it.
    // Shift+Up/Down peeks at a description; before every handler that takes
    // Up/Down.
    if (acc::peek::HandleShiftArrow(e.code, e.val, panel, ChainFocusOn(panel))) {
        return TrackPress(e, 1);
    }
    if (int rv = 0; acc::menus::abilities::HandleInput(e.n, e.mgr, panel, e.code, e.val, rv)) {
        return TrackPress(e, rv);
    }
    if (int rv = 0; acc::menus::listbox::TryHandleInput(e.n, e.mgr, panel, e.code, e.val, rv)) {
        return TrackPress(e, rv);
    }
    // Before chain navigation, so Up/Down in edit mode re-reads the text.
    if (int rv = 0; acc::menus::editbox::TryHandleInput(e.n, e.mgr, panel, e.code, e.val, rv)) {
        return TrackPress(e, rv);
    }
    // The two chart grids return without the press tracker, as they always
    // have.
    if (int rv = 0;
        acc::menus::chargen_feats::HandleInput(e.n, e.mgr, panel, e.code, e.val, rv)) {
        return rv;
    }
    if (int rv = 0;
        acc::menus::powers_levelup::HandleInput(e.n, e.mgr, panel, e.code, e.val, rv)) {
        return rv;
    }

    // The cycle keys are in-world only; the handler declines in menus.
    bool consumed = acc::cycle_input::TryHandleEvent(e.code, e.val);

    if (int rv = 0; acc::pazaak::TryHandleInput(panel, e.code, e.val, rv)) {
        return TrackPress(e, rv);
    }
    if (int rv = 0; acc::menus::pazaakdeck::TryHandleInput(panel, e.code, e.val, rv)) {
        return TrackPress(e, rv);
    }
    if (int rv = 0; acc::menus::galaxymap::TryHandleInput(panel, e.code, e.val, rv)) {
        return TrackPress(e, rv);
    }
    if (int rv = 0; acc::menus::keymap::HandleInput(panel, e.code, e.val, rv)) {
        return TrackPress(e, rv);
    }

    // The generic chain: Enter, Up/Down/Home/End, Left/Right, Esc. Esc on a
    // sub-screen passes to the engine, which closes it; the drill flag
    // clears in ResolveActivePanel.
    acc::menus::chain::HandleEnterActivation(panel, e.code, e.val, consumed);
    acc::menus::chain::HandleNavStep(panel, e.code, e.val, consumed);
    acc::menus::chain::HandleLeftRight(panel, e.code, e.val, consumed);
    acc::menus::chain::HandleEsc(panel, e.code, e.val, consumed);

    // Keyboard liveness (engine_input.h): a key can reach the manager
    // without reaching the in-world handler, and either proves the engine's
    // keyboard is alive.
    if (e.val == 128) NoteEngineKeyEvent();

    LogEvent(e, consumed ? " CONSUMED" : "");
    return TrackPress(e, consumed ? 1 : 0);
}

// Per-tick Win32 hotkey router - see input_poll_router.h for what this is
// and why it is not merged into input_pipeline.cpp.
//
// The gate ORDER in PollHotkey is behaviour. Do not reorder without an
// in-game input pass.

#include "input_poll_router.h"
#include "interact_internal.h"
#include "interact_dispatch.h"

#include <windows.h>
#include <cstdio>

#include "combat_diag.h"     // LeaderIsAttacking: R while already attacking
#include "combat_query.h"
#include "combat_queue.h"
#include "engine_area.h"
#include "engine_input.h"
#include "engine_levelup.h"
#include "engine_offsets.h"
#include "engine_panels.h"
#include "engine_picker.h"
#include "engine_player.h"
#include "examine_view.h"
#include "exit_memory.h"
#include "floor_puzzle.h"
#include "hotkeys.h"
#include "input_pipeline.h"
#include "log.h"
#include "party_status.h"
#include "speech.h"
#include "strings.h"
#include "unified_action_menu.h"

namespace acc::input_poll {

using namespace acc::interact;

namespace {

namespace hk = acc::hotkeys;
using acc::strings::Get;
using acc::strings::Id;

// This tick's rising edges, from the hotkey registry (hotkeys.h).
struct Edges {
    bool enterForce;     // Shift+Enter or Shift+R: two keys, one action
    bool enter;          // plain or forced
    bool up, down, left, right, home, end, esc;
    bool target[3];      // bare 1..3
    bool personal[6];    // bare 4..9
    bool openPersonal[6];  // Shift+4..9
    bool openTarget[3];    // Shift+1..3
    bool levelUp;

    bool NumberOpener() const {
        for (bool b : openPersonal) if (b) return true;
        for (bool b : openTarget) if (b) return true;
        return false;
    }
};

Edges ReadEdges() {
    using A = hk::Action;
    Edges e{};
    const bool enterPlain = hk::Pressed(A::InteractTarget);
    e.enterForce = hk::Pressed(A::InteractForceRadial) ||
                   hk::Pressed(A::InteractForceRadialSecondary);
    e.enter = enterPlain || e.enterForce;
    e.up    = hk::Pressed(A::NavUp);
    e.down  = hk::Pressed(A::NavDown);
    e.left  = hk::Pressed(A::NavLeft);
    e.right = hk::Pressed(A::NavRight);
    e.home  = hk::Pressed(A::NavHome);
    e.end   = hk::Pressed(A::NavEnd);
    const A kTarget[3] = {A::TargetKey1, A::TargetKey2, A::TargetKey3};
    const A kPersonal[6] = {A::PersonalKey1, A::PersonalKey2, A::PersonalKey3,
                            A::PersonalKey4, A::PersonalKey5, A::PersonalKey6};
    const A kOpenPersonal[6] = {A::ActionBarOpen1, A::ActionBarOpen2, A::ActionBarOpen3,
                                A::ActionBarOpen4, A::ActionBarOpen5, A::ActionBarOpen6};
    const A kOpenTarget[3] = {A::TargetActionOpen1, A::TargetActionOpen2,
                              A::TargetActionOpen3};
    for (int i = 0; i < 3; ++i) e.target[i] = hk::Pressed(kTarget[i]);
    for (int i = 0; i < 6; ++i) e.personal[i] = hk::Pressed(kPersonal[i]);
    for (int i = 0; i < 6; ++i) e.openPersonal[i] = hk::Pressed(kOpenPersonal[i]);
    for (int i = 0; i < 3; ++i) e.openTarget[i] = hk::Pressed(kOpenTarget[i]);
    e.levelUp = hk::Pressed(A::LevelUpOpen);
    e.esc     = hk::Pressed(A::SubmenuEsc);
    return e;
}

void Say(Id id) { acc::speech::Speak(Get(id), /*interrupt=*/true); }

// Shift+L opens the engine's level-up panel directly
// (CGuiInGame::ShowLevelUpGUI), from any screen.
void OpenLevelUp() {
    // ShowLevelUpGUI allocates a fresh panel on every call without an
    // "already open" check, so key repeat stacks modals the player cannot
    // unwind.
    if (acc::engine::HasActiveLevelUpPanel()) {
        Say(Id::LevelUpAlreadyOpen);
        acclog::Write("Interact", "Shift+L -> already-open guard, skipping dispatch");
        return;
    }
    // The engine's own btn_levelup state: without it the forced open let
    // the player level up endlessly.
    if (!acc::engine_levelup::PlayerCanLevelUp()) {
        Say(Id::LevelUpNotReady);
        acclog::Write("Interact",
                      "Shift+L -> CanLevelUp=0, refusing (not enough XP / capped)");
        return;
    }
    Say(Id::LevelUpOpen);
    const bool ok = acc::engine_levelup::TriggerLevelUp();
    acclog::Write("Interact", "Shift+L -> [%s] level-up dispatch ok=%d",
                  Get(Id::LevelUpOpen), ok ? 1 : 0);
    if (!ok) Say(Id::LevelUpFailed);
}

// The Shift+number openers of the action menu, and Shift+L.
void HandleOpeners(const Edges& e) {
    // Over the combat queue (Shift+H) an opener switches to the action menu:
    // the queue closes first so it stops shadowing input. The owner-tracked
    // overlay pause keeps the world frozen across the switch.
    if (e.NumberOpener() && acc::combat::queue::IsActive()) {
        acclog::Write("Interact", "Shift+number over combat queue - closing queue, "
                      "switching to action menu");
        acc::combat::queue::ForceDisarm("switch-to-action-menu");
    }
    // Shift+4..7 open personal columns 0..3 (Friendly Force, Medical,
    // Miscellaneous, Explosives), linear like the engine's own
    // DoPersonalAction for bare 4..7. Shift+8 and Shift+9 are the mod's own
    // openers for KOTOR 2's fifth and sixth columns (combat behaviour,
    // lightsaber and Force forms); empty on KOTOR 1, where the open says so.
    for (int i = 0; i < 6; ++i) {
        if (e.openPersonal[i]) acc::unified_menu::OpenPersonal(i);
    }
    // Shift+1..3 open target-action rows 0..2, as DoTargetAction maps them.
    for (int i = 0; i < 3; ++i) {
        if (e.openTarget[i]) acc::unified_menu::OpenTarget(i);
    }
    if (e.levelUp) OpenLevelUp();
}

// Bare 1..7: the engine fires the action through its own handler whatever
// we do; this only speaks what was fired. Not gated on the action menu: it
// stays open as a paused queueing surface, bare presses still queue, and
// the menu itself never speaks on number keys. Not while a dialog owns the
// number keys (reply selection): the engine suppresses the action there.
void AnnounceBareKeys(const Edges& e) {
    if (acc::engine::HasActiveDialogPanel()) return;
    for (int i = 0; i < 3; ++i) {
        if (e.target[i]) AnnounceBareTargetKey(i);
    }
    for (int i = 0; i < 4; ++i) {
        if (e.personal[i]) AnnounceBarePersonalKey(i);
    }
    // Bare 8 and 9 FIRE: nothing in the engine dispatches on them.
    if (e.personal[4]) acc::unified_menu::FirePersonal(4);
    if (e.personal[5]) acc::unified_menu::FirePersonal(5);
}

// Feeds the edges to an armed overlay as engine input codes. In-world
// Enter, arrows, Home and End never reach CSWGuiManager (the engine keymap
// drops those unbound scancodes), so the Win32 edges are translated here.
template <typename Handler>
void RouteBasicKeys(const Edges& e, Handler handle) {
    if (e.enter) handle(kInputEnter1, 1);
    if (e.up)    handle(kInputNavUp, 1);
    if (e.down)  handle(kInputNavDown, 1);
    if (e.esc) {
        handle(kInputEsc1, 1);
        acc::input::NoteOverlayEscClosed();
    }
}

// True when an armed overlay owned this tick's keys. The examine view wins
// over the combat queue, which wins over the action menu.
bool RouteToOverlay(const Edges& e) {
    if (acc::examine_view::IsActive()) {
        RouteBasicKeys(e, acc::examine_view::HandleInputEvent);
        return true;
    }
    if (acc::combat::queue::IsActive()) {
        RouteBasicKeys(e, acc::combat::queue::HandleInputEvent);
        return true;
    }
    if (acc::unified_menu::IsActive() && !acc::unified_menu::IsSuspended()) {
        // Ctrl+Home / Ctrl+End jump between categories, plain within one.
        const bool ctrl = hk::CtrlHeld();
        const auto handle = acc::unified_menu::HandleInputEvent;
        if (e.left)  handle(kInputNavLeft, 1);
        if (e.right) handle(kInputNavRight, 1);
        if (e.home)  handle(ctrl ? kInputCatFirst : kInputHome, 1);
        if (e.end)   handle(ctrl ? kInputCatLast : kInputEnd, 1);
        RouteBasicKeys(e, handle);
        return true;
    }
    return false;
}

// Bare R: the engine's "default action on current target" fires on its own;
// this adds the spoken pre-roll from the live descriptor, read-only. Not
// where another consumer owns R (dialog repeat, the floor puzzle's board
// read) or a UI panel is in front.
void AnnounceNativeDefaultAction() {
    if (!hk::Pressed(hk::Action::DialogRepeatLine) ||
        acc::engine::HasActiveDialogPanel() || acc::floor_puzzle::IsActive() ||
        acc::engine::IsForegroundUiBlocking()) {
        return;
    }
    acc::picker::ActionSnapshot snap = {};
    if (!acc::picker::ReadCurrent(&snap) || !snap.label[0]) {
        acclog::Write("Interact", "R native-default announce - no valid descriptor "
                      "(snap.valid=%d); silent", snap.valid ? 1 : 0);
        return;
    }
    char name[128] = "";
    if (void* obj = acc::engine::ResolveClientObjectHandle(snap.target_id)) {
        acc::engine::GetObjectName(obj, name, sizeof(name));
    }
    char msg[192];
    // Already attacking this target: R only queues the same attack again
    // (the engine attacks every round by itself), so say that instead of
    // "Attack X" on every press.
    if (name[0] && acc::engine::IsAnyPartyMemberInCombat() &&
        acc::combat::queue::LeaderIsAttacking(snap.target_id)) {
        std::snprintf(msg, sizeof(msg), Get(Id::FmtAlreadyAttacking), name);
    } else if (name[0]) {
        std::snprintf(msg, sizeof(msg), Get(Id::FmtInteractEngine), snap.label, name);
    } else {
        std::snprintf(msg, sizeof(msg), "%s", snap.label);
    }
    acc::speech::Speak(msg, /*interrupt=*/true);
    acclog::Write("Interact", "R native-default announce -> [%s] action=0x%x "
                  "target=0x%08x", msg, snap.action_id, snap.target_id);
}

// True when no blocking UI panel is in front, so Enter may act in the
// world. GetPlayerPosition only says we are in-world, not whether a panel
// routes the input; the predicate is the one the Tab leader announce uses.
bool EnterMayActInWorld(const char* keyTag) {
    using acc::engine::PanelKindName;
    using acc::engine::UiBlockReason;
    acc::engine::UiBlockState ui;
    if (acc::engine::IsForegroundUiBlocking(&ui)) {
        switch (ui.reason) {
        case UiBlockReason::DialogInStack:
            acclog::Write("Interact", "%s gate -- BLOCKED, dialog panel in stack", keyTag);
            break;
        case UiBlockReason::ForegroundModal:
            acclog::Write("Interact", "%s gate -- BLOCKED, fg=%p kind=%s "
                          "(modal_stack[%d] top)", keyTag, ui.fgPanel,
                          PanelKindName(ui.fgKind), ui.modalStackTop);
            break;
        case UiBlockReason::ForegroundBlockingKind:
            acclog::Write("Interact", "%s gate -- BLOCKED, fg=%p kind=%s", keyTag,
                          ui.fgPanel, PanelKindName(ui.fgKind));
            break;
        default:
            break;
        }
        return false;
    }
    if (!ui.fgPanel) return true;
    acclog::Write("Interact", "%s gate -- ALLOW, fg=%p kind=%s", keyTag, ui.fgPanel,
                  PanelKindName(ui.fgKind));
    // Gate-gap audit, a log line and not a gate. Few kinds legitimately sit
    // in front of live world play (HUD, pause overlay, bark, fade, Unknown).
    // Any other kind here means a screen routes its own keys AND we queue an
    // engine action on the same press: the shape that cost KOTOR 2's
    // prologue its module transition (the galaxy map).
    if (!acc::engine::IsWorldLevelForegroundKind(ui.fgKind)) {
        acclog::Write("Interact",
                      "%s GATE GAP? -- dispatching in-world over fg=%p kind=%s "
                      "(not a world-level surface; if this press also drove the "
                      "panel, that kind belongs in IsForegroundUiBlocking)",
                      keyTag, ui.fgPanel, PanelKindName(ui.fgKind));
    }
    return true;
}

// Enter and Shift+Enter in the world: interact with the narrated target.
void HandleWorldEnter(bool forceRadial) {
    const char* keyTag = forceRadial ? "Shift+Enter" : "Enter";
    if (!EnterMayActInWorld(keyTag)) return;

    // The Enter that confirmed a save-name edit box: that popup classifies
    // as kind Unknown and passes the gate, and the same Enter would queue a
    // dialogue on the narrated target, to fire when the world unpauses. The
    // edit box monitor set the latch earlier this tick; single-shot.
    if (acc::input::ConsumeEditboxSubmitLatch()) {
        acclog::Write("Interact", "%s swallowed -- editbox submit closed a modal this "
                      "tick (save-name confirm); not dispatching interact", keyTag);
        return;
    }
    OnInteract(forceRadial);
}

}  // namespace

void PollHotkey() {
    const Edges e = ReadEdges();

    // Like the game's own menu hotkeys (J, I, M), an action-menu opener
    // pressed over the in-game menu switches: the menu closes back to the
    // world, the way each tab's own Escape does, and the opener then arms
    // this same tick. Message boxes, dialogs and stores stay blockers, as
    // for the engine's hotkeys.
    if ((e.enterForce || e.NumberOpener()) && ShouldSwitchFromInGameMenu()) {
        acclog::Write("Interact",
                      "action-menu opener over in-game menu - closing to world (switch)");
        acc::engine::CloseInGameMenuToWorld();
    }

    // The player object, not the GUI status: true across the close above.
    Vector unused;
    const bool inWorld = acc::engine::GetPlayerPosition(unused);

    // The action menu holds a world pause and takes its keys through this
    // poll without owning an engine panel. When the engine pushes a real
    // panel over it (a hotkey sub-screen, a message box), the menu must
    // stop consuming the same keys: it SUSPENDS while a blocker is up and
    // resumes at the same position after, like a native menu under a popup.
    acc::unified_menu::SetForegroundBlocked(acc::engine::IsForegroundUiBlocking());

    if (inWorld) {
        HandleOpeners(e);
        AnnounceBareKeys(e);
    }

    // Features that poll their own hotkey and gate themselves.
    acc::examine_view::PollWin32Hotkey();
    acc::combat::queue::PollWin32Hotkey();             // Shift+H
    acc::combat::query::PollWin32SelfStatusHotkey();   // H
    acc::exit_memory::PollOverviewHotkey();            // Ctrl+N
    acc::exit_memory::PollWalkHotkeys();               // Ctrl+Shift+N, Ctrl+L
    acc::party_status::PollHotkey();                   // Ctrl+H

    if (!inWorld) return;
    if (RouteToOverlay(e)) return;
    AnnounceNativeDefaultAction();
    if (e.enter) HandleWorldEnter(e.enterForce);
}

}  // namespace acc::input_poll

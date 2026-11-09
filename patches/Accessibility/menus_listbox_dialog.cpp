// Dialogue reply lists: the conversation, cinematic and computer panels.

#include "menus_listbox_internal.h"

namespace acc::menus::listbox {

namespace {

// ============================================================================
// CSWGuiDialog — replies_listbox @+0x19c4. Phase 1D of the combat-system
// plan. Same shape applies to DialogCinematic / DialogCinematicCopy /
// DialogComputer / DialogComputerCamera variants — registered as four
// matchers all pointing at the shared listbox locator.
// ============================================================================

bool DialogCinematicMatches(void* p) {
    return IdentifyPanel(p) == PanelKind::DialogCinematic;
}

bool DialogCinematicCopyMatches(void* p) {
    return IdentifyPanel(p) == PanelKind::DialogCinematicCopy;
}

bool DialogComputerMatches(void* p) {
    return IdentifyPanel(p) == PanelKind::DialogComputer;
}

bool DialogComputerCameraMatches(void* p) {
    return IdentifyPanel(p) == PanelKind::DialogComputerCamera;
}

void* DialogFindRepliesLb(void* p) {
    // See InGameMessagesFindLb. This is the exact site that crashed KOTOR 2 on
    // the first in-world arrow key: kDialogRepliesListBoxOffset is still Todo,
    // and `panel + poison` passed the caller's `lb &&` check.
    return acc::off::Ptr(p, kDialogRepliesListBoxOffset);
}

// Dialogue reply panels intentionally have NO announce callback: the specs
// exist to DRIVE the listbox cursor (DriveListBoxSelection) and CONSUME the
// nav keys when the dialog panel is the routed panel. The actual speech is
// owned solely by MonitorDialogReplies (a per-tick poll), which catches both
// engine-driven changes (the first reply when a node opens) and our cursor
// moves. Having a second announce here read every nav keypress was the
// "options read twice" bug; its is_active@+0x4c "unavailable" gate also
// mis-fired on selectable replies (the field tracks highlight state on a
// reply row, not selectability), which was the "shown disabled but actually
// available" bug. Both are gone now that the poll is the only speaker.
//
// IMPORTANT scope note (2026-08-25): in CINEMATIC dialogs these specs only
// ever see nav keys while the NPC line is playing (the dialog panel is
// foreground then). Once the replies are live, the entry-text overlay panel
// is foreground, the chain declines it, and the keys fall through to the
// ENGINE, whose native CSWGuiListBox keyboard handler moves the selection —
// that is the path that navigates the live reply list. It only works while
// the cursor stays parked off the list; the engine repositions the cursor
// between dialogue nodes, which is why MonitorDialogReplies re-parks on
// every new reply set (see docs/known-issues.md "K1 dialogue replies",
// userlogs/zack078dialoguesnotspeaking). The earlier claim here that "the
// engine does not navigate the reply listbox by keyboard" was true only for
// the droid/computer interfaces.

// The engine populates the CGuiInGame reply store (SetReplyData) only at the
// reply break — when the entry's VO ends and the choices actually appear. A
// press before that has nothing readable to land on: the listbox rows that
// exist mid-VO are the previous node's leftovers whose labels are already
// cleared, and ReadDialogReplyText misses for every index. Gate nav on the
// store instead of the row count.
bool DialogRepliesLive(void* /*panel*/) {
    return acc::engine::ReadDialogReplyCount() > 0;
}

// All four dialog variants take the identical spec — same reply-listbox
// locator, no announce (see above), no Enter/Esc/title handling. Only the
// log tag and the panel-kind matcher differ, so build them from one shape
// rather than four hand-copied field lists that could drift apart.
constexpr ListBoxPanelSpec MakeDialogSpec(const char* logTag,
                                          bool (*matches)(void* panel)) {
    return {
        /*logTag*/                  logTag,
        /*matches*/                 matches,
        /*armed*/                   nullptr,
        /*resetStale*/              nullptr,
        /*findListBox*/             DialogFindRepliesLb,
        /*minSel*/                  0,
        /*announce*/                nullptr,
        /*enrichRow*/               nullptr,
        /*logExtra*/                nullptr,
        /*onEnter*/                 nullptr,
        /*onEsc*/                   nullptr,
        /*titleOverride*/           nullptr,
        /*emptyStateId*/            acc::strings::Id::DialogNoReplyOptions,
        /*alwaysReturnFromHandler*/ false,
        /*minSelFn*/                nullptr,
        /*useEngineSelect*/         false,
        /*navLive*/                 DialogRepliesLive,
    };
}

}  // namespace

constexpr ListBoxPanelSpec kDialogCinematicSpec =
    MakeDialogSpec("DialogCinematic", DialogCinematicMatches);
constexpr ListBoxPanelSpec kDialogCinematicCopySpec =
    MakeDialogSpec("DialogCinematicCopy", DialogCinematicCopyMatches);
constexpr ListBoxPanelSpec kDialogComputerSpec =
    MakeDialogSpec("DialogComputer", DialogComputerMatches);
constexpr ListBoxPanelSpec kDialogComputerCameraSpec =
    MakeDialogSpec("DialogComputerCamera", DialogComputerCameraMatches);

}  // namespace acc::menus::listbox

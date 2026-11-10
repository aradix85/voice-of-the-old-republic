// The general per-tick menu monitors: TickGeneralMonitors, the focus
// monitor and AnnounceControl, which keeps the focus monitor's last-seen
// state in sync when it speaks. Also the cursor park the keyboard-driven
// listbox screens share.

#include "menus_monitors_internal.h"

#include "engine_offsets.h"
#include "engine_reads.h"
#include "menus.h"
#include "menus_chargen_attr.h"
#include "menus_chargen_skills.h"
#include "menus_chain.h"
#include "menus_extract.h"
#include "menus_listbox.h"
#include "speech.h"
#include "tutorial_hints.h"
#include "tutorial_popup.h"

// ---------------------------------------------------------------------------
// Cursor park — shared by every keyboard-driven listbox screen.
//
// The engine slaves a listbox's selection_index to whatever row the mouse
// cursor hovers: CSWGuiListBox::HandleMouseMove does HitCheckMouseLocal ->
// SetSelectedControl on every mouse move the manager processes, and
// CSWGuiDialog::SetReplies calls HandleMouseMove explicitly when a node opens.
// That silently overwrites the keyboard-driven selection we write in
// DriveListBoxSelection whenever the OS cursor happens to sit over a row —
// which is exactly what a widescreen/HD geometry produces: the resting cursor
// lands on the list where a 4:3 cursor sits in empty space, so the hover-select
// keeps snapping selection back to the row under it. Symptom is always the
// same: only the two rows adjacent to the hovered one are reachable, and the
// hovered row itself is never announced (observed first on the droid repair
// submenu, then on the Key bindings screen). The workbench/equip pickers hit
// the same engine behaviour and defeat it by parking the cursor off their
// listbox (ParkPickerCursorOffList in menus_listbox.cpp).
//
// We warp to the top-left letterbox corner rather than a computed off-list
// point on the panel because these panels are label-rich (the computer dialog's
// UpdateSkills paints skill/resource readouts near the replies) and
// MoveMouseToPosition's hover->active promotion crashes on a label — the corner
// is empty on every panel and trivially off any centred list at any resolution.
// It also keeps the park from promoting a button to active and firing a
// spurious focus announce. Runs only from the per-tick monitors (Update tick),
// never the input hook, because MoveMouseToPosition recurses through the hover
// pipeline. Self-correcting for mouse users: moving the mouse un-parks it.
//
// The park must clear the engine's screen-edge camera-turn band. A cursor left
// sitting in that band makes UpdateCamera accumulate-turn every frame once world
// control returns, so the camera spins endlessly with no input and no cure but
// restarting. x=2 did exactly that after every conversation, just outside
// camera_spin_diag's guard band (kMinBandPx); keep kCursorParkX comfortably
// above that floor. y stays at the very top.
namespace acc::menus::detail {

namespace {
constexpr int kCursorParkX = 24;
constexpr int kCursorParkY = 2;
}  // namespace

bool ParkCursorToCorner(const char* tag) {
    void* gm = GetGuiManager();
    if (!gm) return false;
    reinterpret_cast<PFN_MoveMouseToPosition>(kAddrMoveMouseToPosition)(
        gm, kCursorParkX, kCursorParkY);
    acclog::Write(tag,
                  "park cursor to top-left corner (%d,%d) — neutralises engine "
                  "hover-select, clear of the camera edge-turn band",
                  kCursorParkX, kCursorParkY);
    return true;
}

}  // namespace acc::menus::detail

namespace acc::menus::monitors {

namespace {

// ============================================================================
// Focus monitor — re-extracts the focused chain entry's text each tick and
// speaks the diff. State (last-seen control + text) is shared with
// AnnounceControl, which keeps it in sync after voluntary speak events.
// ============================================================================
void* s_focusMonitorControl = nullptr;
char  s_focusMonitorText[acc::menus::detail::kAnnounceTextMax] = {0};

void MonitorFocusedControl() {
    // Eager rebind for the chargen wizard steps. Each of these panels is
    // pushed by its parent sub-menu ("Custom Character") WITHOUT firing
    // OnSetActiveControl, so the chain stays bound to the parent until the
    // user's first arrow press. HandleNavStep then rebinds AND steps in a
    // single call, so the first Down lands on index 1 and the initially-
    // anchored entry (index 0) is never announced — the user had to press
    // Up to hear it. Rebinding here the moment the panel is foreground lets
    // the focus monitor below announce the anchored entry on open, before
    // any keypress. A no-op once bound (fg == g_chainPanel) so it never
    // re-anchors while the user navigates the panel.
    //
    // Deliberately an explicit vtable list rather than "any fg that isn't
    // the chain panel": the drill router in menus_dispatch retargets the
    // chain AWAY from fg on purpose when the user has Entered a sub-screen
    // from the in-game strip, and a blanket rebind here would fight it every
    // tick. PortraitCharGen was the first of these found; Abilities and
    // Skills joined it from the KOTOR 2 chargen round (their index-0 row —
    // Strength / Computer Use — was silent on open).
    if (void* mgr = GetGuiManager()) {
        void* fg = GetForegroundPanel(mgr);
        if (fg && fg != acc::menus::chain::g_chainPanel) {
            void* vt = nullptr;  // guarded: fg may already be torn down
            if (acc::engine::TryReadPtr(fg, 0, &vt)) {
                uintptr_t vtaddr = reinterpret_cast<uintptr_t>(vt);
                if (vtaddr == kVtableCSWGuiPortraitCharGen ||
                    vtaddr == kVtableCSWGuiAbilitiesCharGen ||
                    vtaddr == kVtableCSWGuiSkillsCharGen) {
                    acc::menus::chain::RebindChain(fg);
                }
            }
        }
    }

    if (acc::menus::chain::g_chainCount <= 0 ||
        acc::menus::chain::g_chainIndex < 0 ||
        acc::menus::chain::g_chainIndex >= acc::menus::chain::g_chainCount) {
        return;
    }
    if (acc::menus::chain::g_chainPanel != g_currentPanel) {
        // g_currentPanel is written only from OnSetActiveControl, so it goes
        // stale for every panel the engine pushes without firing a focus
        // event — CSWGuiPortraitCharGen on both games, and on KOTOR 2 the
        // whole chargen wizard (abchrgen_p, skchrgen_p) as well, all pushed
        // straight out of the parent step-list's button handler. The gate
        // then suppressed the text-change re-announce for the entire visit:
        // on KOTOR 2 that is why Left/Right on an attribute or skill
        // silently changed the value.
        //
        // The real invariant the old PortraitCharGen special-case was
        // approximating: the chain is authoritative when it is bound to the
        // panel the user is actually looking at. Ask the manager for its
        // foreground panel — the same source the routing layer picks the
        // chain panel from — instead of naming one vtable.
        void* chainPanel = acc::menus::chain::g_chainPanel;
        if (!chainPanel) return;
        void* mgr = GetGuiManager();
        if (!mgr || GetForegroundPanel(mgr) != chainPanel) return;
    }
    void* focused = acc::menus::chain::g_chain[acc::menus::chain::g_chainIndex].control;
    if (!focused) return;

    char text[acc::menus::detail::kAnnounceTextMax];
    const char* source = acc::menus::extract::FromControl(
        focused, text, sizeof(text), acc::menus::chain::g_chainPanel);
    if (!source) return;

    // Tutorial popup message row: AnnounceControl already speaks the keyboard
    // hint on arrow-nav. This monitor would otherwise re-read the raw mouse text
    // off the control (the 5th announce path) — and copying the hint into the
    // 256-char `text` buffer would clip a long hint — so just skip it here.
    //   * Surface 2 (synthetic Trask popup): decide by identity — the popup's
    //     only single-row listbox is its message.
    //   * Surface 1 (mapped tutorial.2da popup): decide by text — the row still
    //     carries its mouse-worded strref, which HintForMouseText recognises.
    // Without the Surface-1 arm this monitor re-spoke the raw mouse line a tick
    // after AnnounceControl substituted the hint (the double-read on the
    // targeting / equip popups).
    if (IsListBox(focused) &&
        (acc::tutorial_popup::SyntheticActive() ||
         acc::tutorial_hints::HintForMouseText(text) != nullptr)) {
        return;
    }

    if (focused == s_focusMonitorControl) {
        if (strncmp(s_focusMonitorText, text, sizeof(s_focusMonitorText)) != 0) {
            // Chargen Attribute / Skills panels: when a +/- press
            // changes the focused row's value, override the default
            // "{label}, {new_value}" re-announce with the panel's
            // own value-change format. Returns true when a panel-
            // specific override fired; we still update the monitor's
            // last-text snapshot so the next tick doesn't re-fire on
            // the same diff.
            void* chainPanel = acc::menus::chain::g_chainPanel;
            bool handled =
                acc::menus::chargen_attr::AnnounceValueChange(
                    chainPanel, focused) ||
                acc::menus::chargen_skills::AnnounceValueChange(
                    chainPanel, focused);
            // While the equip picker is open, the focused control is the slot
            // button UNDERNEATH it, and its text changes on every arrow press —
            // the engine equips each row as you land on it (its live preview;
            // OnItemSelected is the row's select handler and calls EquipItem
            // directly). So arrowing down one row produced two lines: the
            // picker's own "Quarterstaff, 2 of 48" and then this monitor's
            // "Right Weapon, Quarterstaff", sometimes three when the swap flashed
            // through an empty slot first. The second line repeats an item name the user has just been told and
            // describes a state that is not committed yet.
            //
            // Suppress the speech but keep updating the snapshot, so the diff
            // that matters still fires the moment the picker closes: Escape
            // reverts to the item you started with and this announces exactly
            // that, which is the user's confirmation that the cancel took.
            bool equipPreview = acc::menus::listbox::IsEquipPickerArmed();
            if (!handled && !equipPreview) {
                acc::speech::Speak(text, /*interrupt=*/false);
            }
            strncpy_s(s_focusMonitorText, text, _TRUNCATE);
            acclog::Write("Monitor", "focused=%p text changed -> \"%s\"%s",
                          focused, text,
                          handled ? " (chargen override)"
                                  : (equipPreview ? " (equip preview, silent)"
                                                  : ""));
        }
    } else {
        s_focusMonitorControl = focused;
        strncpy_s(s_focusMonitorText, text, _TRUNCATE);
        // Route through channel-0 dedup rather than speaking unconditionally:
        // this monitor is the LAST of the three announce paths to run, so if
        // one of the others already said this line, saying it again is a
        // double-announce.
        //
        // That is the chargen class-icon case. The per-icon label cache
        // starts cold, so on a first visit AnnounceControl's FromControl
        // returns nullptr on the nav itself (SetActive src=none) and it
        // exits via its class-icon early-out — without speaking, and without
        // priming this monitor's last-seen control the way its success
        // branch does. The cursor warp then makes active_control the icon,
        // DrainPendingAnnounce's FromControl fills the cache and speaks, and
        // this monitor — still unprimed — spoke the same line again. On a
        // revisit AnnounceControl wins with a warm cache and does the
        // priming, which is why the double only ever appeared on the first
        // pass over each icon and looked direction-dependent.
        //
        // SpeakIfChanged also marks, so the dedup stays primed for whichever
        // path runs next.
        acc::menus::SpeakIfChanged(/*channel=*/0, text);
        acclog::Write("Monitor", "focus changed -> %p text=\"%s\"",
                      focused, text);
    }
}

}  // namespace

void AnnounceControl(void* control) {
    if (!control) return;

    // Multi-row listbox guard. The engine fires SetActiveControl on the
    // listbox container as part of panel construction (auto-focus on
    // open) before the user has navigated anywhere. The container itself
    // is not a focusable terminal — rows are exposed via the
    // ListBoxPanelSpec / chain handlers as the user arrows into them.
    // Speaking the container's "control N" fallback on panel open is
    // pure noise; the panel-name announce already covers the open event.
    //
    // Single-row listboxes (CSWGuiMessageBox-style modals carrying the
    // dialog text in a single row) DO need to speak — FromControl
    // returns the row text and we fall through to the success path.
    //
    // The rule that a fallback is never silenced is about user-driven
    // navigation, where a missing announce makes focus feel frozen;
    // engine-driven panel-open auto-focus on a container is a different
    // class of event.
    const bool isListBoxContainer = IsListBox(control);
    if (acc::menus::detail::IsMultiRowListBox(control)) return;

    char text[acc::menus::detail::kAnnounceTextMax];
    // Pass the chain panel so the extraction is identical to
    // MonitorFocusedControl's (which also passes g_chainPanel). FromControl's
    // panel-scoped enrichment — notably the InGameLevelUp "unavailable"
    // suffix keyed on the per-step enabled flag — only fires when it knows the
    // owning panel; announcing without it produced a plain label here and a
    // suffixed one from the monitor a tick later, which the monitor heard as a
    // text change and spoke twice. Any panel whose per-kind extraction
    // differed would already have double-announced (monitor vs. here), so this
    // only newly aligns the level-up case.
    const char* source = acc::menus::extract::FromControl(
        control, text, sizeof(text), acc::menus::chain::g_chainPanel);
    if (source) {
        // Tutorial popup message: arrow-navigating onto the message row speaks
        // the keyboard hint instead of the raw mouse text (buttons read their
        // own labels). Decide by IDENTITY, not text: the synthetic Trask popup's
        // message is its only single-row listbox, so a listbox control here IS
        // the message — speak the hint. (Surface-1 popups still match by the
        // tutorial.2da mouse strref via HintForMouseText.)
        const char* spoken = nullptr;
        if (acc::tutorial_popup::SyntheticActive() && isListBoxContainer)
            spoken = acc::tutorial_popup::SyntheticHint();
        if (!spoken) spoken = acc::tutorial_hints::HintForMouseText(text);
        if (!spoken) spoken = text;
        acc::speech::Speak(spoken, /*interrupt=*/false);
        // Prime channel-0 dedup so the engine's post-nav SetActive echo
        // (which lands in the pending-announce slot and gets drained next
        // tick) sees the same text as last-spoken and stays silent.
        acc::menus::MarkSpoken(/*channel=*/0, spoken);
        s_focusMonitorControl = control;
        strncpy_s(s_focusMonitorText, spoken, _TRUNCATE);
        return;
    }
    void* livePanel = CurrentPanelIfLive();
    if (livePanel && acc::menus::detail::IsClassSelectionIcon(livePanel, control)) {
        return;
    }
    // A listbox container with no extractable text is the engine's panel-open
    // auto-focus on an empty / not-yet-populated list (e.g. the workbench
    // LB_ITEMS at controls[0], id 0, before the slot is chosen), not a
    // navigable terminal. The multi-row guard above only catches populated
    // lists; an empty one fell through to "control N" — the "control 0" spoken
    // on workbench open. Containers carrying real text (message-box single-row
    // listboxes) already returned via the FromControl success path above, so
    // suppressing here only drops contentless containers, never a real focus
    // event (the "never silence the fallback" rule is about navigable controls).
    if (isListBoxContainer) {
        return;
    }
    // Guarded: the control can be freed between capture and this read (the
    // GetControlCenter crash class). A dead control still announces — the
    // fallback is never silenced — just with an unknowable id.
    uint32_t id = 0;
    if (!acc::engine::TryReadU32(control, kControlIdOffset, &id)) {
        id = static_cast<uint32_t>(-1);
    }
    char placeholder[64];
    snprintf(placeholder, sizeof(placeholder), "control %d", static_cast<int>(id));
    acc::speech::Speak(placeholder, /*interrupt=*/false);
}

void TickGeneralMonitors() {
    // First: drain the pending-announce slot. Multiple SetActive events
    // fired within this tick collapsed to the last write — speak it now
    // (or stay silent if the voluntary AnnounceControl path already spoke
    // matching text and primed the channel-0 dedup).
    acc::menus::DrainPendingAnnounce();
    MonitorFocusedControl();
    MonitorInventoryListPopulation();
    MonitorPanelContents();
    MonitorDialogReplies();
    // Detect our synthetic Trask-line tutorial popup closing → unpause + reset.
    acc::tutorial_popup::PollDismiss();
    // Re-assert chargen Attribute panel's selected_ability against chain
    // focus. The engine's cursor-warp → OnEnterPointsButton path silently
    // overwrites this field between our chain-step sync and the queued
    // FireActivate from Left/Right; this monitor runs before
    // TickPendingOps in the same OnUpdate, so OnPlusButton reads the
    // correct value when the FireActivate fires. No-op on every other
    // panel.
    acc::menus::chargen_attr::SyncSelectedAbilityFromChainFocus();
    // Same defense on the Skills panel — different field
    // (selected_skill_index) but same engine-overwrite race.
    acc::menus::chargen_skills::SyncSelectedSkillFromChainFocus();
}

}  // namespace acc::menus::monitors

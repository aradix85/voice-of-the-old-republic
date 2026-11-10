// The two SetActiveControl hooks, the engine's focus-change signals: the
// panel hook (CSWGuiPanel) feeds the pending-announce slot through
// menus_focus, the listbox hook (CSWGuiListBox) speaks the focused row. Both
// stay silent through the GUI rebuild burst of a cutscene or a load.

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "bringup_announce.h"
#include "engine_area.h"
#include "engine_offsets.h"
#include "engine_panels.h"
#include "engine_reads.h"
#include "log.h"
#include "menus.h"
#include "menus_chain.h"
#include "menus_chargen_attr.h"
#include "menus_chargen_skills.h"
#include "menus_extract.h"
#include "menus_focus.h"
#include "menus_internal.h"
#include "menus_monitors.h"
#include "speech.h"
#include "transitions.h"
#include "tutorial_hints.h"
#include "tutorial_popup.h"

using namespace acc::engine;

namespace {

// Some cutscenes play as several movie segments with the movie window
// destroyed in between, where IsMoviePlaying() reads false for a moment; the
// tail covers those gaps and the world rebuild after the last segment.
constexpr unsigned kMovieBurstTailMs = 2000;

// The tail applies in-game only: at startup a movie ends right before the
// main menu, whose first-sight announce rides the SetActive the tail would
// swallow.
bool InMovieBurstWindow() {
    if (acc::bringup_announce::IsMoviePlaying()) return true;
    return acc::bringup_announce::IsInputPumpLive() &&
           acc::bringup_announce::MovieRecentlyPlaying(kMovieBurstTailMs);
}

// Why both hooks stay silent now, or nullptr. A cutscene and every module or
// save-game load build and activate a storm of panels; speaking them would
// talk over the movie or the loading screen and mark them first-seen, so the
// player's first real visit would be silent. IsLoadingSaveGame covers the
// in-game load, where the old world is still live and the module latch never
// armed.
const char* BurstSuppression() {
    if (InMovieBurstWindow()) return "movie foreground";
    if (acc::transitions::IsModuleLoadPending() || IsLoadingSaveGame()) {
        return "save-load / module load";
    }
    return nullptr;
}

int ControlId(void* control) {
    return *reinterpret_cast<int*>(static_cast<unsigned char*>(control) + kControlIdOffset);
}

// The main menu's first SetActive is the engine's auto-focus on New Game; a
// second one comes from the engine's own mouse or keyboard focus path and so
// proves the input pump is live. Keyboard-only players get the same handoff
// from the first consumed key in the manager hook (menus_dispatch.cpp),
// since the chain's synthetic mouse-over fires no SetActiveControl.
void NoteMainMenuSetActive() {
    static int s_count = 0;
    if (++s_count == 2) {
        acclog::BringupMark("main_menu_input_pump_live");
        acc::bringup_announce::NotifyInputPumpLive();
    }
}

// selection_index is -1 in scroll mode (bit_flags & 0x200), the row in
// selection mode.
void LogListBoxCursor(void* listBox, int rowCount) {
    auto* base = static_cast<unsigned char*>(listBox);
    acclog::Write("Menus.ListBox", "cursor list=%p sel=%d top=%d perPage=%d size=%d flags=0x%x",
                  listBox,
                  *reinterpret_cast<short*>(base + kListBoxSelectionIndexOffset),
                  *reinterpret_cast<short*>(base + kListBoxTopVisibleIndexOffset),
                  *reinterpret_cast<short*>(base + kListBoxItemsPerPageOffset),
                  rowCount, *reinterpret_cast<uint32_t*>(base + kListBoxBitFlagsOffset));
}

// The first listbox event after a panel change probes the focused panel for
// the Options layout (listbox at [0] plus a button cluster), whose tab
// buttons need the chain step's Y compensation.
void LatchTabsCluster() {
    using namespace acc::menus::chain;
    if (!g_currentPanel || g_tabbedPanel == g_currentPanel) return;
    int tabsStart = -1, tabsCount = 0;
    if (!DetectTabsCluster(g_currentPanel, tabsStart, tabsCount)) return;
    g_tabbedPanel = g_currentPanel;
    g_tabsStart   = tabsStart;
    g_tabsCount   = tabsCount;
    acclog::Write("Menus.Tabs", "detected panel=%p tabsStart=%d tabsCount=%d",
                  g_currentPanel, tabsStart, tabsCount);
}

// Why a row with text stays silent, or nullptr.
const char* SilencedRowReason(void* listBox, const char* text, int rowCount) {
    // The chargen description listboxes: their hover-driven text belongs to
    // the neighbouring row (the cursor warp's hit-test lands one off), so the
    // chain step speaks the focused row's description itself.
    if (acc::menus::chargen_skills::IsChargenSkillsDescriptionListbox(listBox)) {
        return "chargen-skills description (the chain step reads it)";
    }
    if (acc::menus::chargen_attr::IsChargenAttributesDescriptionListbox(listBox)) {
        return "chargen-attr description (the chain step reads it)";
    }
    // Options-style blob: every setting joined by '\n' in one label row.
    if (strchr(text, '\n')) return "multi-line blob";
    // In-game sub-screens pair their button chain with a one-row description
    // listbox the engine re-activates on every focus change.
    void* livePanel = CurrentPanelIfLive();
    if (rowCount == 1 && livePanel &&
        acc::menus::monitors::IsInGameSubScreenKind(IdentifyPanel(livePanel))) {
        return "sub-screen description listbox";
    }
    return nullptr;
}

}  // namespace

// CSWGuiPanel::SetActiveControl, hooked mid-function at 0x0040a638 with
// EDI = the panel and ESI = the new active control (null when the panel drops
// its selection). Fires once per real focus change: arrows, mouse and code.
// KOTOR 2 reaches this through OnSetActiveControlK2 (menus_focus_k2.cpp),
// which does the frame arithmetic and its cursor warp first.
extern "C" void __cdecl OnSetActiveControl(void* panel, void* newControl) {
    EnsureSpeechInitialized();
    static int n = 0;
    ++n;

    if (const char* why = BurstSuppression()) {
        acclog::Write("Menus.SetActive", "#%d panel=%p — suppressed (%s)", n, panel, why);
        return;
    }
    const PanelKind kind = IdentifyPanel(panel);
    if (kind == PanelKind::MainMenu) NoteMainMenuSetActive();

    acc::menus::focus::PrefillClassIconCacheOnTransition(panel, newControl);
    acc::menus::focus::UpdateFocusedPanelState(panel);
    acc::menus::focus::WalkAndCaptureOnFirstSight(panel);

    // The InGameMenu icon strip is never a menu of its own (hotkeys and Tab
    // drill straight into its sub-screens); its SetActives would speak
    // "Equipment" and "Messages" over the sub-screen's title.
    if (kind == PanelKind::InGameMenu) {
        acclog::Write("Menus.SetActive", "#%d panel=%p new=%p id=%d (InGameMenu strip — "
                      "speech suppressed)", n, panel, newControl,
                      newControl ? ControlId(newControl) : -1);
        return;
    }

    acc::menus::focus::SpeakPanelTitleOnFirstSight(panel);
    if (!newControl) {
        acclog::Write("Menus.SetActive", "#%d panel=%p newControl=NULL", n, panel);
        return;
    }
    acc::menus::focus::AnnounceNewFocusedControl(n, panel, newControl);
}

// CSWGuiListBox::SetActiveControl(CSWGuiControl* row, int param_2), hooked
// mid-function at 0x0041c16b, where all three arguments sit in registers:
// the 8-byte cut is PUSH EAX, PUSH ECX and LEA ECX,[ESI+0x29c], all
// position-independent. Row focus inside a listbox never bubbles up to the
// panel's SetActiveControl, so the pickers and the save list need this.
extern "C" void __cdecl OnListBoxSetActiveControl(void* listBox, void* newRow, int param2) {
    EnsureSpeechInitialized();
    static int n = 0;
    ++n;

    if (const char* why = BurstSuppression()) {
        acclog::Write("Menus.ListBox", "SetActive #%d list=%p — suppressed (%s)",
                      n, listBox, why);
        return;
    }

    static void* s_lastListBox = nullptr;
    if (listBox && listBox != s_lastListBox) {
        s_lastListBox = listBox;
        acc::menus::chain::WalkChildren("Menus.ListBox", listBox, kListBoxControlsOffset);
    }
    const int rowCount = ReadListBoxRowCount(listBox);
    if (listBox) LogListBoxCursor(listBox, rowCount);

    if (!newRow) {
        acclog::Write("Menus.ListBox", "SetActive #%d list=%p newRow=NULL p2=%d",
                      n, listBox, param2);
        return;
    }

    const int id = ControlId(newRow);
    char text[256];
    const char* source = acc::menus::extract::FromControl(newRow, text, sizeof(text));
    if (!source) {
        char vtbl[160];
        DumpControlVtable(newRow, vtbl, sizeof(vtbl));
        acclog::Write("Menus.ListBox", "SetActive #%d list=%p row=%p id=%d p2=%d src=none %s",
                      n, listBox, newRow, id, param2, vtbl);
        // A one-row listbox is a description blob beside a chain panel that
        // alternates between text and none as the chain moves; only real
        // lists get the placeholder.
        if (rowCount > 1) {
            char placeholder[64];
            snprintf(placeholder, sizeof(placeholder), "row %d", id);
            acc::speech::Speak(placeholder, /*interrupt=*/false);
        }
        return;
    }
    acclog::Write("Menus.ListBox", "SetActive #%d list=%p row=%p id=%d p2=%d src=%s text=\"%s\"",
                  n, listBox, newRow, id, param2, source, text);

    LatchTabsCluster();
    if (const char* why = SilencedRowReason(listBox, text, rowCount)) {
        acclog::Write("Menus.ListBox", "silenced: %s", why);
        return;
    }

    // The tutorial popup's message row: its wording is mouse-centric, so the
    // keyboard hint is spoken in its place (the synthetic Trask popup shows
    // the original mouse strref, so its hint comes from the popup module).
    // Matched on text because this fires before the popup is in panels[].
    // Spoken here rather than by the content monitor, as on KOTOR 2 the
    // monitor's re-extract reads nothing after the first popup; the dedup
    // collapses the two paths to one line.
    const char* hint = acc::tutorial_popup::SyntheticActive()
                           ? acc::tutorial_popup::SyntheticHint()
                           : acc::tutorial_hints::HintForMouseText(text);
    acc::menus::SpeakIfChanged(/*channel=*/1, (hint && hint[0]) ? hint : text);
}

// The two SetActiveControl hooks, the engine's focus-change signals: the
// panel hook (CSWGuiPanel) feeds the pending-announce slot through
// menus_focus, the listbox hook (CSWGuiListBox) speaks the focused row. Both
// stay silent through the GUI rebuild burst of a cutscene or a load.

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

using acc::menus::chain::g_tabbedPanel;
using acc::menus::chain::g_tabsStart;
using acc::menus::chain::g_tabsCount;
using acc::menus::chain::DetectTabsCluster;
using acc::menus::chain::WalkChildren;

using acc::menus::focus::PrefillClassIconCacheOnTransition;
using acc::menus::focus::UpdateFocusedPanelState;
using acc::menus::focus::WalkAndCaptureOnFirstSight;
using acc::menus::focus::SpeakPanelTitleOnFirstSight;
using acc::menus::focus::AnnounceNewFocusedControl;
using acc::menus::SpeakIfChanged;

namespace {

// How long after a cutscene movie ends we keep suppressing the SetActive
// rebuild burst. The engine plays some cutscenes (the Peragus docking stunt)
// as several movie segments and destroys the movie window between them; the
// burst leaked in a sub-second gap where IsMoviePlaying() read false. Two
// seconds covers those gaps and the tail while the world rebuilds after the
// final segment — not long enough to swallow anything the player would open
// themselves.
constexpr unsigned kMovieBurstTailMs = 2000;

// True while the engine-GUI SetActive rebuild burst must stay silent because a
// cutscene movie is on screen — or was, within the last kMovieBurstTailMs.
//
// The bare-presence half fires in every phase (startup intros included). The
// just-ended tail is gated on IsInputPumpLive() so it only applies in-game:
// during startup a movie ends immediately before the main menu appears, and
// the main-menu first-sight announce rides the very SetActive we would then
// suppress. In-game there is no such announce racing the tail.
bool InMovieBurstWindow() {
    if (acc::bringup_announce::IsMoviePlaying()) return true;
    return acc::bringup_announce::IsInputPumpLive() &&
           acc::bringup_announce::MovieRecentlyPlaying(kMovieBurstTailMs);
}

// Why both SetActiveControl hooks stay silent right now, or nullptr.
//
// During a cutscene-load and on any save-game or module load the engine
// constructs and activates a storm of GUI panels — in-game HUD, store,
// options strip, equip screens, rebuilt listboxes — firing SetActiveControl
// on each. Announcing them talks over the movie or the loading screen AND
// would mark those panels first-seen, so they'd stay silent on the player's
// first real visit. The engine re-fires SetActiveControl on genuine
// interaction afterwards, so first-sight and focus speech resume cleanly.
//
// The load has two signals: IsModuleLoadPending covers module/door
// transitions (set by the SetMoveToModuleString detour); IsLoadingSaveGame
// is the engine's own load_from_savegame flag, true across any save-game
// restore — crucially the IN-GAME load, where the old world is still live
// so the module latch never armed.
const char* BurstSuppression() {
    if (InMovieBurstWindow()) return "movie foreground";
    if (acc::transitions::IsModuleLoadPending() || acc::engine::IsLoadingSaveGame()) {
        return "save-load / module load";
    }
    return nullptr;
}

int ListBoxRowCount(void* listBox) {
    auto* rows = reinterpret_cast<CExoArrayList*>(
        reinterpret_cast<unsigned char*>(listBox) + kListBoxControlsOffset);
    return rows->size;
}

}  // namespace

// CSWGuiPanel::SetActiveControl — hooked mid-function at 0x0040a638.
// At hook entry: EDI = this (the panel), ESI = param_1 (the new active
// control, possibly null when the panel is deactivating selection).
//
// This is the canonical focus-change signal: fires once per actual change,
// covers arrow-key nav + mouse + programmatic.
//
// KOTOR 2 does not hook this directly — its hook is OnSetActiveControlK2 in
// menus_focus_k2.cpp, which does the frame arithmetic and the KOTOR 2 cursor
// warp and then calls this. On KOTOR 2 the CGuiInGame-slot in-game screens
// classify as Unknown until that slot table is ported; IdentifyPanel falls
// through to the structural / vtable identification for the rest.
extern "C" void __cdecl OnSetActiveControl(void* panel, void* newControl) {
    EnsureSpeechInitialized();
    static int n = 0;
    ++n;

    if (const char* why = BurstSuppression()) {
        acclog::Write("Menus.SetActive", "#%d panel=%p — suppressed (%s)", n, panel, why);
        return;
    }

    // Bringup handoff (secondary signal — mouse path only). The engine
    // fires one SetActive immediately after panel construction for its
    // auto-focused button ("New Game"); a genuine 2nd SetActive on the
    // MainMenu comes from the engine's own mouse-over/keyboard focus path.
    // This clears the bring-up nag for mouse users, but NOT for keyboard-
    // only players: our chain navigates via synthetic mouse-over, which
    // fires no SetActiveControl. The primary, keyboard-reachable handoff is
    // the first consumed key in the manager hook (see OnHandleInputEvent).
    if (IdentifyPanel(panel) == PanelKind::MainMenu) {
        static int mainMenuSetActiveCount = 0;
        if (++mainMenuSetActiveCount == 2) {
            acclog::BringupMark("main_menu_input_pump_live");
            acc::bringup_announce::NotifyInputPumpLive();
        }
    }

    PrefillClassIconCacheOnTransition(panel, newControl);
    UpdateFocusedPanelState(panel);
    WalkAndCaptureOnFirstSight(panel);

    // The InGameMenu strip is never surfaced as a navigable menu — hotkeys +
    // Tab/Shift+Tab drill the user directly into its sub-screens. The engine
    // still fires SetActiveControl on it (panel-open, and "Messages" once per
    // first open), which would speak "Equipment" + "Messages" over the
    // sub-screen's own title and focus speech. The diagnostic walk above
    // stays; only speech is gated.
    if (IdentifyPanel(panel) == PanelKind::InGameMenu) {
        if (newControl) {
            int sid = *reinterpret_cast<int*>(
                reinterpret_cast<unsigned char*>(newControl) + kControlIdOffset);
            acclog::Write("Menus.SetActive",
                          "#%d panel=%p new=%p id=%d (InGameMenu strip — "
                          "speech suppressed)",
                          n, panel, newControl, sid);
        } else {
            acclog::Write("Menus.SetActive",
                          "#%d panel=%p newControl=NULL (InGameMenu strip)",
                          n, panel);
        }
        return;
    }

    SpeakPanelTitleOnFirstSight(panel);

    if (!newControl) {
        acclog::Write("Menus.SetActive", "#%d panel=%p newControl=NULL", n, panel);
        return;
    }

    AnnounceNewFocusedControl(n, panel, newControl);
}

// CSWGuiListBox::SetActiveControl — hooked mid-function at 0x0041c16b.
// Function entry per Lane's SARIF:
//   void __thiscall CSWGuiListBox::SetActiveControl(CSWGuiControl* param_1, int param_2)
//
// Bytes from 0x0041c160 (DumpBytes.java):
//   8b 44 24 08          MOV EAX, [ESP+8]   ; param_2 (int) before push
//   56                   PUSH ESI
//   8b f1                MOV ESI, ECX       ; this → ESI
//   8b 4c 24 08          MOV ECX, [ESP+8]   ; param_1 (post-push, was [ESP+4])
//   50 51 8d 8e 9c 02 00 00     ← hook here, all three args in registers
//   50                   PUSH EAX           ; param_2
//   51                   PUSH ECX           ; param_1
//   8d 8e 9c 02 00 00    LEA  ECX, [ESI+0x29c]  ; embedded sub-object
//
// Cut covers PUSH EAX (1) + PUSH ECX (1) + complete LEA (6) = 8 bytes. All
// three instructions are position-independent → safe to relocate.
//
// Listbox row navigation does NOT bubble up to CSWGuiPanel::SetActiveControl,
// so without this hook we miss every per-row focus event inside listboxes
// (race / class / portrait pickers in chargen, save-game list, etc.).
extern "C" void __cdecl OnListBoxSetActiveControl(void* listBox, void* newRow,
                                                  int param2) {
    EnsureSpeechInitialized();

    static int n = 0;
    ++n;

    if (const char* why = BurstSuppression()) {
        acclog::Write("Menus.ListBox", "SetActive #%d list=%p — suppressed (%s)",
                      n, listBox, why);
        return;
    }

    // First event for a previously-unseen listbox: dump every row control.
    static void* s_lastListBox = nullptr;
    if (listBox && listBox != s_lastListBox) {
        s_lastListBox = listBox;
        WalkChildren("Menus.ListBox", listBox, kListBoxControlsOffset);
    }

    // The listbox's internal cursor and flags. selection_index distinguishes
    // scroll-mode (-1, set when bit_flags & 0x200) from selection-mode (>=0);
    // a multi-line-blob settings listbox has 1 row though the user sees 8
    // visual lines.
    const int rowCount = listBox ? ListBoxRowCount(listBox) : 0;
    if (listBox) {
        auto* base = reinterpret_cast<unsigned char*>(listBox);
        short itemsPerPage = *reinterpret_cast<short*>(
            base + kListBoxItemsPerPageOffset);
        short selIdx       = *reinterpret_cast<short*>(
            base + kListBoxSelectionIndexOffset);
        short topVisible   = *reinterpret_cast<short*>(
            base + kListBoxTopVisibleIndexOffset);
        uint32_t bitFlags  = *reinterpret_cast<uint32_t*>(
            base + kListBoxBitFlagsOffset);
        acclog::Write("Menus.ListBox", "cursor list=%p sel=%d top=%d perPage=%d "
                      "size=%d flags=0x%x",
                      listBox, selIdx, topVisible, itemsPerPage,
                      rowCount, bitFlags);
    }

    if (!newRow) {
        acclog::Write("Menus.ListBox", "SetActive #%d list=%p newRow=NULL p2=%d",
                      n, listBox, param2);
        return;
    }

    int id = *reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(newRow) + kControlIdOffset);

    char text[256];
    const char* source = acc::menus::extract::FromControl(newRow, text, sizeof(text));

    if (!source) {
        char vtbl[160];
        DumpControlVtable(newRow, vtbl, sizeof(vtbl));
        acclog::Write("Menus.ListBox", "SetActive #%d list=%p row=%p id=%d "
                      "p2=%d src=none %s",
                      n, listBox, newRow, id, param2, vtbl);
        // No placeholder for single-row listboxes: those are description
        // blobs next to a chain panel, which the engine re-activates as the
        // user navigates the chain, alternating between text and none; "row
        // 0" five times per step is noise. Real multi-row listboxes
        // (save-game list, chargen pickers) keep the fallback so an
        // extraction failure on one row still announces.
        if (rowCount > 1) {
            char placeholder[64];
            snprintf(placeholder, sizeof(placeholder), "row %d", id);
            acc::speech::Speak(placeholder, /*interrupt=*/false);
        }
        return;
    }

    acclog::Write("Menus.ListBox", "SetActive #%d list=%p row=%p id=%d "
                  "p2=%d src=%s text=\"%s\"",
                  n, listBox, newRow, id, param2, source, text);

    // Lazy tabbed-mode detection: the first listbox event after a panel
    // change probes whether the focused panel has the Options-style
    // listbox-at-[0] + button-cluster layout, for the chain-step Y-offset
    // compensation of tab buttons.
    if (g_currentPanel && g_tabbedPanel != g_currentPanel) {
        int tabsStart = -1, tabsCount = 0;
        if (DetectTabsCluster(g_currentPanel, tabsStart, tabsCount)) {
            g_tabbedPanel = g_currentPanel;
            g_tabsStart   = tabsStart;
            g_tabsCount   = tabsCount;
            acclog::Write("Menus.Tabs", "detected panel=%p tabsStart=%d tabsCount=%d",
                          g_currentPanel, tabsStart, tabsCount);
        }
    }

    if (acc::menus::chargen_skills::IsChargenSkillsDescriptionListbox(listBox)) {
        // Chargen Skills description listbox: the engine's hover-driven
        // population is off by one here (the cursor warp's hit-test resolves
        // to skill_labels[i-1] whatever the Y compensation), so its text is
        // the wrong row. The chain-step handler speaks the focused row's
        // description from skill_descriptions[i] instead.
        acclog::Write("Menus.ListBox",
                      "chargen-skills description silenced "
                      "(handled by chain-step direct read)");
        return;
    }
    if (acc::menus::chargen_attr::IsChargenAttributesDescriptionListbox(listBox)) {
        // Same for chargen attributes: the hover-driven population depends
        // on the resolution (the hit-test can resolve to the neighbouring
        // ability). The chain-step handler speaks the focused ability's
        // description through a direct OnEnterPointsButton call.
        acclog::Write("Menus.ListBox",
                      "chargen-attr description silenced "
                      "(handled by chain-step direct read)");
        return;
    }
    if (strchr(text, '\n')) {
        // Multi-line listbox blob (Options-style: all settings concatenated
        // by '\n' into a single CSWGuiLabel row). Always silenced — bulk
        // enumeration is too noisy.
        int lines = 1;
        for (const char* p = text; *p; ++p) if (*p == '\n') ++lines;
        acclog::Write("Menus.ListBox", "blob silenced; lines=%d", lines);
        return;
    }
    // In-game sub-screens (InGameOptions, InGameInventory, …) pair their
    // button chain with a single-row description listbox whose text updates
    // on every focus change, and the engine re-activates its row each time.
    // Silenced; a sub-screen that wants its description speaks it from the
    // chain-step extract path (like chargen skills).
    void* livePanel = CurrentPanelIfLive();
    if (rowCount == 1 && livePanel &&
        acc::menus::monitors::IsInGameSubScreenKind(IdentifyPanel(livePanel))) {
        acclog::Write("Menus.ListBox",
                      "sub-screen description listbox silenced (panel kind=%s)",
                      PanelKindName(IdentifyPanel(livePanel)));
        return;
    }
    // The tutorial popup's single-row message listbox: the vanilla wording is
    // mouse-centric, so the keyboard hint is spoken in its place when the row
    // maps to one, otherwise the row's own text. Matched on text, not panel
    // identity, because this fires before the popup registers in panels[]
    // (tutorial_hints.cpp). The synthetic Trask popup shows the original mouse
    // strref as its visible text, so its hint comes from the popup module.
    //
    // Spoken HERE rather than left to the content-fingerprint monitor: this
    // hook holds the text, while the monitor has to re-extract it, and on
    // KOTOR 2 that extraction returns nothing for every popup after the first
    // (the message listbox reads as src=none). Both paths resolve the same
    // hint string, so the announce dedup collapses them to one line.
    const char* hint =
        acc::tutorial_popup::SyntheticActive()
            ? acc::tutorial_popup::SyntheticHint()
            : acc::tutorial_hints::HintForMouseText(text);
    SpeakIfChanged(/*channel=*/1, (hint && hint[0]) ? hint : text);
}

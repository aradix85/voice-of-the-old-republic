// The menu side's own state and hooks: the speech dedup channels, the
// focused panel, the sub-screen drill flag, the two SetActiveControl hooks
// (panel and listbox) with the pending-announce slot they feed, and the
// per-tick glue core_tick calls (ValidatePanels, TickMonitors,
// PollHomeEndKeys, TickPendingOps).

#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "bringup_announce.h"
#include "core_tick.h"
#include "engine_area.h"
#include "engine_input.h"
#include "engine_manager.h"
#include "engine_offsets.h"
#include "engine_panels.h"
#include "engine_reads.h"
#include "hotkeys.h"
#include "log.h"
#include "menus.h"
#include "menus_chain.h"
#include "menus_chargen_attr.h"
#include "menus_chargen_skills.h"
#include "menus_crafting.h"
#include "menus_editbox.h"
#include "menus_extract.h"
#include "menus_focus.h"
#include "menus_galaxymap.h"
#include "menus_internal.h"
#include "menus_keymap.h"
#include "menus_listbox.h"
#include "menus_monitors.h"
#include "menus_pending.h"
#include "menus_store.h"
#include "speech.h"
#include "transitions.h"
#include "tutorial_hints.h"
#include "tutorial_popup.h"

using namespace acc::engine;

using acc::menus::chain::g_chain;
using acc::menus::chain::g_chainPanel;
using acc::menus::chain::g_chainIndex;
using acc::menus::chain::g_chainCount;
using acc::menus::chain::g_tabbedPanel;
using acc::menus::chain::g_tabsStart;
using acc::menus::chain::g_tabsCount;
using acc::menus::chain::ValidateTabbedPanel;
using acc::menus::chain::ValidateChainPanel;
using acc::menus::chain::DetectTabsCluster;
using acc::menus::chain::WalkChildren;

using acc::menus::focus::PrefillClassIconCacheOnTransition;
using acc::menus::focus::UpdateFocusedPanelState;
using acc::menus::focus::WalkAndCaptureOnFirstSight;
using acc::menus::focus::SpeakPanelTitleOnFirstSight;
using acc::menus::focus::AnnounceNewFocusedControl;

// Defined in menus_dispatch.cpp. PollHomeEndKeys re-enters the GUI input
// pipeline through it to deliver the Home/End edges the engine keymap drops.
extern "C" int __cdecl OnHandleInputEvent(void* thisPtr, int param_1, int param_2);

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

// ---------------------------------------------------------------------------
// Speech dedup channels.
//
// Speak `text` only if it differs from what we last spoke on this channel.
// Dedup is the only filter: speech is queued (interrupt=false), because focus
// events fire ~10/sec while panels initialize and interrupting cut every line
// off mid-word; NVDA finishes each line at its own pace and the user can skip
// with NVDA's own key.
//
// Channels keep dedup state independent so a listbox row update doesn't
// silence the parent panel's announcement and vice-versa:
//   0 = panel SetActiveControl  (slot drain + voluntary AnnounceControl)
//   1 = listbox row SetActiveControl
//
// The focus monitor's AnnounceControl calls MarkSpoken(0, text) after it
// speaks, so the slot drain suppresses the engine's echo of the same nav.
namespace acc::menus {

namespace {
// Sized with the announce buffers: SpeakIfChanged compares the WHOLE stored
// string, so a snapshot shorter than the text it guards would report two
// different lines as identical whenever they share a long prefix.
char s_lastSpoken[2][acc::menus::detail::kAnnounceTextMax] = {{0}, {0}};
DWORD s_lastSpokenAt[2] = {0, 0};
}

void MarkSpoken(int channel, const char* text) {
    if (channel < 0 || channel >= 2 || !text) return;
    strncpy_s(s_lastSpoken[channel], text, _TRUNCATE);
    s_lastSpokenAt[channel] = acc::tick::NowMs();
}

void SpeakIfChanged(int channel, const char* text, unsigned repeatAfterMs) {
    if (channel < 0 || channel >= 2 || !text) return;
    if (strncmp(s_lastSpoken[channel], text,
                sizeof(s_lastSpoken[channel])) == 0) {
        // Same text. Suppress unless the caller declared it re-speakable and
        // enough time has passed — see the repeatAfterMs note in menus.h.
        if (repeatAfterMs == 0) return;
        if (acc::tick::NowMs() - s_lastSpokenAt[channel] < repeatAfterMs) return;
    }
    strncpy_s(s_lastSpoken[channel], text, _TRUNCATE);
    s_lastSpokenAt[channel] = acc::tick::NowMs();
    acc::speech::Speak(text, /*interrupt=*/false);
}

bool AlreadySpokenOnAnyChannel(const char* text) {
    if (!text) return false;
    for (int c = 0; c < 2; ++c) {
        if (strncmp(s_lastSpoken[c], text, sizeof(s_lastSpoken[c])) == 0) {
            return true;
        }
    }
    return false;
}

}  // namespace acc::menus

using acc::menus::SpeakIfChanged;

bool acc::menus::detail::IsMultiRowListBox(void* control) {
    if (!IsListBox(control)) return false;
    auto* rows = reinterpret_cast<CExoArrayList*>(
        reinterpret_cast<unsigned char*>(control) + kListBoxControlsOffset);
    return rows->data && rows->size > 1;
}

// The focused panel: set by OnSetActiveControl, read across chain code,
// monitors and extract::FromControl (extern in menus_internal.h).
void* g_currentPanel = nullptr;

// See menus_internal.h for why every deref of g_currentPanel has to come
// through here. IsPanelInManager is a pointer-equality scan over panels[] —
// it never dereferences the candidate, so it is safe on a wild pointer.
void* CurrentPanelIfLive() {
    return acc::engine::IsPanelInManager(g_currentPanel) ? g_currentPanel
                                                         : nullptr;
}

// Sub-screen drill state. The InGameMenu icon strip is kept in foreground by
// the engine: each icon's onClick (OnInvButtonPressed @0x624d10 etc.) jumps
// into CGuiInGame::SwitchToSWInGameGui @0x62cf10, which calls AddPanel for the
// new sub-screen and then SendPanelToBack on it — the strip stays on top.
// Without intervention our chain therefore keeps targeting the strip's 8
// icons and the user can never reach the sub-screen's content (item rows,
// quest rows, settings buttons).
//
// Drill model: opening a sub-screen arms this flag (AnnounceNewSubScreens).
// The chain-target router in OnHandleInputEvent then prefers
// FindActiveSubScreenPanel() over the engine's foreground when fg is the
// strip — so arrows step through the sub-screen instead. Esc clears the flag
// (returns to strip nav). The flag also self-clears when the sub-screen
// leaves panels[].
//
// Override is gated on fg-is-the-strip: while a tutorial modal or an
// Options sub-tab is on top, fg is something else and we route to that
// directly (no double-override). Once the modal/sub-tab closes and fg
// returns to the strip, the override re-engages. The Esc / drill-routing
// gates in menus_dispatch.cpp read and clear it (menus_internal.h).
bool g_drilledIntoSubScreen = false;

// Pending-announce slot for the panel-focus path. AnnounceNewFocusedControl
// (menus_focus.cpp) writes it on every event; DrainPendingAnnounce reads and
// clears it once per tick. Multiple intra-tick events overwrite the slot, a
// natural last-write-wins coalesce:
//
//   1. A triple-burst panel-open (NULL → first child → engine's actual
//      default focus) would otherwise produce two utterances ("OK, Cancel"
//      on MessageBox open); the third write lands before the next tick.
//   2. A voluntary-nav echo (chain step + cursor warp → the engine echoes a
//      SetActive on the same control) is silenced by the channel-0 dedup
//      AnnounceControl primed with MarkSpoken.
namespace acc::menus {
void* s_pendingAnnouncePanel   = nullptr;
void* s_pendingAnnounceControl = nullptr;

// Set while PollHomeEndKeys is synthesising a call to OnHandleInputEvent.
// The engine drops KEYBOARD_HOME(32) / KEYBOARD_END(33) before our manager
// hook (no [Keymapping] action targets them), so we Win32-poll them and
// re-enter the input pipeline ourselves. The synthesised call has no
// matching engine-sent release, so the press-release pair tracker inside
// OnHandleInputEvent skips its update when this flag is set — otherwise
// a synthesised non-consumed press would clobber the tracker for a
// previous real consumed press (Enter activations, etc.). Read in
// menus_dispatch.cpp (menus_internal.h).
bool s_synthesizedNav = false;
}

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

namespace acc::menus {

void ValidatePanels() {
    // If the engine freed the panel DetectTabsCluster last latched onto,
    // drop the stale pointer before any input handler can deref it.
    ValidateTabbedPanel();
    // Same hazard for g_chainPanel / g_chain[].control: when Esc on the
    // Main-Menu Options strip (or any other panel our Esc gate doesn't
    // catch) falls through to the engine's native handler, the engine
    // destroys the panel and its children. The chain still references the
    // freed buttons; next tick MonitorFocusedControl dereferences one and
    // FromControl's SEH-caught AV interacts with /GS to fastfail (a crash
    // dump showed the freed Auto-Pause button). A queued FireActivate
    // already invalidates the chain; the engine-handled Esc path has no
    // such hook, so the guard is generic here.
    ValidateChainPanel();
}

void TickMonitors() {
    // Store runs FIRST so its trade-outcome speech ("Sold" / "Bought" /
    // "Cannot …") and chain rebind land before the focus monitor's per-tick
    // re-extract of the focused chain row. Otherwise a successful sell
    // speaks the next item's name (the focus monitor saw the row's text
    // change as the listbox repopulated) BEFORE the outcome — as if the
    // announce were about a different item than the one just sold.
    acc::menus::store::TickMonitorMode();
    // K2 workbench/crafting watcher — same ordering rationale as Store:
    // its view-flip speech + chain rebind land before the focus monitor's
    // per-tick re-extract of the focused chain row.
    acc::menus::crafting::Tick();
    acc::menus::monitors::TickGeneralMonitors();
    acc::menus::listbox::TickListboxMonitors();
    acc::menus::editbox::TickEditboxMonitors();
    acc::menus::galaxymap::Tick();
    acc::menus::keymap::Tick();
}

void PollHomeEndKeys() {
    bool home = acc::hotkeys::Pressed(acc::hotkeys::Action::NavHome);
    bool end_ = acc::hotkeys::Pressed(acc::hotkeys::Action::NavEnd);
    if (!home && !end_) return;

    // Both pressed in the same tick: prefer Home (the user is unlikely to
    // hit both intentionally; defer End to the next press).
    int code = home ? kInputHome : kInputEnd;

    void* mgr = GetGuiManager();
    if (!mgr) {
        acclog::Write("Menus.PollHomeEnd",
                      "%s pressed but mgr=NULL; ignored",
                      home ? "Home" : "End");
        return;
    }

    // Synthesise the manager dispatch through our own hook. The hook
    // re-enters listbox dispatch / editbox dispatch / chain nav exactly
    // as a real engine-routed keypress would. s_synthesizedNav suppresses
    // the press-release tracker write so an unconsumed Home/End can't
    // clobber a still-pending real consumed press (e.g. an Enter that
    // hasn't received its release yet).
    s_synthesizedNav = true;
    ::OnHandleInputEvent(mgr, code, /*param_2=*/1);
    s_synthesizedNav = false;
}

// Drain the menu-side pending-op queue (menus_pending). Called from
// core_tick::Dispatch after all monitors have run, which keeps deep engine
// re-entry off the input-hook stack.
void TickPendingOps() {
    pending::Drain(GetGuiManager());
}

// Drain the pending-announce slot. Called early in TickMonitors so any
// intra-tick burst of SetActive events (panel-open NULL/first/settled
// triple, cursor-warp echo after voluntary nav) collapses to one
// announcement of the final settled focus. Channel-0 dedup short-circuits
// when the voluntary AnnounceControl path already spoke the same text.
//
// Chain-coherence drop: if the slot points at a chain entry on the bound
// panel that is NOT g_chainIndex (i.e. the engine fired SetActive on a
// sibling control after a voluntary chain step), suppress speech. This is
// the InGameOptions cursor-warp hit-test miss pattern — chain steps to
// "Gameplay" (index 2), engine's own DOWN handler / hit-test miss fires
// SetActive on "Load Game" (chain[0]) instead. The chain handler's
// AnnounceControl has already spoken the intended target; drop the
// engine's wrong-sibling echo so the user doesn't hear two button names.
//
// Doesn't suppress: legitimate user-driven focus changes (mouse hover /
// click on a non-chain control or on a chain control via direct cursor) —
// those land on the chain's matching index (handled by SpeakIfChanged
// dedup matching) or on a non-chain control (gate fails open and speaks).
void DrainPendingAnnounce() {
    void* panel   = s_pendingAnnouncePanel;
    void* control = s_pendingAnnounceControl;
    s_pendingAnnouncePanel   = nullptr;
    s_pendingAnnounceControl = nullptr;
    if (!control) return;

    // Panel-open auto-focus on a multi-row listbox container is not user
    // navigation (same guard as AnnounceControl in menus_monitors.cpp).
    const bool isListBoxContainer = IsListBox(control);
    if (acc::menus::detail::IsMultiRowListBox(control)) return;

    if (g_chainPanel == panel && g_chainCount > 0 &&
        g_chainIndex >= 0 && g_chainIndex < g_chainCount &&
        g_chain[g_chainIndex].control != control)
    {
        int slotIdx = -1;
        for (int i = 0; i < g_chainCount; ++i) {
            if (g_chain[i].control == control) { slotIdx = i; break; }
        }
        if (slotIdx >= 0) {
            acclog::Write("Announce",
                          "chain-coherence drop: slot=%p (chain[%d]) != "
                          "chain[%d]=%p; engine sibling-focus echo "
                          "after voluntary nav",
                          control, slotIdx, g_chainIndex,
                          g_chain[g_chainIndex].control);
            return;
        }
    }

    char text[acc::menus::detail::kAnnounceTextMax];
    const char* source = acc::menus::extract::FromControl(
        control, text, sizeof(text), panel);
    if (source) {
        SpeakIfChanged(/*channel=*/0, text);
        return;
    }

    // A listbox container with no extractable text is the engine's panel-open
    // auto-focus on an empty / not-yet-populated list (workbench LB_ITEMS at
    // controls[0], id 0). The multi-row guard above only catches populated
    // lists; an empty one leaked the "control 0" spoken on workbench open.
    // Containers carrying real text returned via the FromControl success path,
    // so this drops only contentless containers, never a navigable control.
    if (isListBoxContainer) return;

    // Chargen class icon with a cold label cache. The icon's class name is
    // only readable once the panel's own OnEnterButton has written
    // class_label, which happens a beat AFTER the panel-open SetActiveControl
    // that queued this announce — so FromControl legitimately has nothing yet
    // (KOTOR 2 loses that race every time, KOTOR 1 happened to win it).
    //
    // Not a silenced fallback: the value arrives a tick later and the focus
    // monitor speaks it then (it has not primed its last-seen control here).
    // Keyed on the drain's own panel, not g_currentPanel.
    if (acc::menus::detail::IsClassSelectionIcon(panel, control)) {
        acclog::Write("Announce",
                      "class-icon %p deferred: label cache cold, focus "
                      "monitor will speak it once the engine fills class_label",
                      control);
        return;
    }

    // No extractable text: a "control N" placeholder, bypassing the dedup on
    // purpose — a repeated "control 11" is better than a focus event that
    // goes silent.
    int id = *reinterpret_cast<int*>(
        reinterpret_cast<unsigned char*>(control) + kControlIdOffset);
    char placeholder[64];
    snprintf(placeholder, sizeof(placeholder), "control %d", id);
    acc::speech::Speak(placeholder, /*interrupt=*/false);
}

void ClearPendingAnnounce() {
    s_pendingAnnouncePanel   = nullptr;
    s_pendingAnnounceControl = nullptr;
}

bool IsDrilledIntoSubScreen() { return g_drilledIntoSubScreen; }
void SetDrilledIntoSubScreen(bool drilled) {
    g_drilledIntoSubScreen = drilled;
}

}  // namespace acc::menus

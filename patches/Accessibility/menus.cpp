// The menu side's own state and per-tick glue: the speech dedup channels,
// the focused panel, the sub-screen drill flag, the pending-announce slot the
// panel SetActiveControl hook feeds (menus_setactive.cpp) and its drain, and
// what core_tick calls each frame (ValidatePanels, TickMonitors,
// PollHomeEndKeys, TickPendingOps).

#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "core_tick.h"
#include "engine_input.h"
#include "engine_manager.h"
#include "engine_offsets.h"
#include "engine_panels.h"
#include "engine_reads.h"
#include "hotkeys.h"
#include "log.h"
#include "menus.h"
#include "menus_chain.h"
#include "menus_crafting.h"
#include "menus_editbox.h"
#include "menus_extract.h"
#include "menus_galaxymap.h"
#include "menus_internal.h"
#include "menus_keymap.h"
#include "menus_listbox.h"
#include "menus_monitors.h"
#include "menus_pending.h"
#include "menus_store.h"
#include "speech.h"

using namespace acc::engine;

using acc::menus::chain::g_chain;
using acc::menus::chain::g_chainPanel;
using acc::menus::chain::g_chainIndex;
using acc::menus::chain::g_chainCount;
using acc::menus::chain::ValidateTabbedPanel;
using acc::menus::chain::ValidateChainPanel;

// Defined in menus_dispatch.cpp. PollHomeEndKeys re-enters the GUI input
// pipeline through it to deliver the Home/End edges the engine keymap drops.
extern "C" int __cdecl OnHandleInputEvent(void* thisPtr, int param_1, int param_2);

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

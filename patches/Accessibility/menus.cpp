// The menu side's own state and per-tick glue: the speech dedup channels,
// the focused panel, the sub-screen drill flag, the pending-announce slot the
// panel SetActiveControl hook feeds (menus_setactive.cpp) and its drain, and
// what core_tick calls each frame (ValidatePanels, TickMonitors,
// PollHomeEndKeys, TickPendingOps).

#include <windows.h>
#include <cstdio>
#include <cstring>

#include "core_tick.h"
#include "engine_input.h"
#include "engine_manager.h"
#include "engine_offsets.h"
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

// Defined in menus_dispatch.cpp. PollHomeEndKeys re-enters the GUI input
// pipeline through it to deliver the Home/End edges the engine keymap drops.
extern "C" int __cdecl OnHandleInputEvent(void* thisPtr, int param_1, int param_2);

// The focused panel (extern in menus_internal.h).
void* g_currentPanel = nullptr;

// IsPanelInManager compares pointers only, so it is safe on a freed panel.
void* CurrentPanelIfLive() {
    return IsPanelInManager(g_currentPanel) ? g_currentPanel : nullptr;
}

// The engine keeps the InGameMenu icon strip in the foreground: each icon's
// onClick reaches CGuiInGame::SwitchToSWInGameGui, which adds the sub-screen
// and then sends it to the back. Opening a sub-screen sets this flag, and
// while the foreground is the strip the chain router in menus_dispatch.cpp
// targets FindActiveSubScreenPanel() instead, so the arrows reach the
// sub-screen's content. A modal or an Options sub-tab on top is the
// foreground itself and needs no override. Esc clears the flag, and so does
// the sub-screen leaving panels[].
bool g_drilledIntoSubScreen = false;

namespace acc::menus {

namespace {
// Channel 0 = the panel focus path (slot drain and the focus monitor's
// AnnounceControl), 1 = listbox rows; independent, so a row update does not
// silence the panel's line. Sized with the announce buffers: the whole
// string is compared, so a shorter snapshot would call two lines with a long
// common prefix identical.
char s_lastSpoken[2][detail::kAnnounceTextMax] = {{0}, {0}};
DWORD s_lastSpokenAt[2] = {0, 0};

bool SameAsLast(int channel, const char* text) {
    return strncmp(s_lastSpoken[channel], text, sizeof(s_lastSpoken[channel])) == 0;
}

void Remember(int channel, const char* text) {
    strncpy_s(s_lastSpoken[channel], text, _TRUNCATE);
    s_lastSpokenAt[channel] = acc::tick::NowMs();
}
}  // namespace

// The panel focus slot, written by AnnounceNewFocusedControl (menus_focus.cpp)
// on every SetActive and drained once per tick: a burst within one tick (the
// NULL / first child / real default triple of a panel open) collapses to its
// last write.
void* s_pendingAnnouncePanel   = nullptr;
void* s_pendingAnnounceControl = nullptr;

// True while PollHomeEndKeys synthesises an OnHandleInputEvent call. That
// call gets no engine release, so the press-release tracker in
// menus_dispatch.cpp skips it rather than overwrite a real pending press.
bool s_synthesizedNav = false;

void MarkSpoken(int channel, const char* text) {
    if (channel < 0 || channel >= 2 || !text) return;
    Remember(channel, text);
}

// Queued, never interrupting: focus events fire about ten times a second
// while a panel builds, and interrupting cut every line mid-word. The dedup
// is the only filter.
void SpeakIfChanged(int channel, const char* text, unsigned repeatAfterMs) {
    if (channel < 0 || channel >= 2 || !text) return;
    if (SameAsLast(channel, text) &&
        (repeatAfterMs == 0 || acc::tick::NowMs() - s_lastSpokenAt[channel] < repeatAfterMs)) {
        return;
    }
    Remember(channel, text);
    acc::speech::Speak(text, /*interrupt=*/false);
}

bool AlreadySpokenOnAnyChannel(const char* text) {
    return text && (SameAsLast(0, text) || SameAsLast(1, text));
}

bool detail::IsMultiRowListBox(void* control) {
    return IsListBox(control) && ReadListBoxRowCount(control) > 1;
}

void ValidatePanels() {
    // When the engine frees a panel (Esc that falls through to its own
    // handler), the latched tabs panel and the chain's controls dangle; a
    // dereference next tick turned into a /GS fastfail.
    chain::ValidateTabbedPanel();
    chain::ValidateChainPanel();
}

void TickMonitors() {
    // Store and the K2 workbench first, so their outcome speech ("Sold") and
    // chain rebind land before the focus monitor re-reads the focused row;
    // otherwise a sale spoke the next item's name before the outcome.
    store::TickMonitorMode();
    crafting::Tick();
    monitors::TickGeneralMonitors();
    listbox::TickListboxMonitors();
    editbox::TickEditboxMonitors();
    galaxymap::Tick();
    keymap::Tick();
}

// The engine drops KEYBOARD_HOME (32) and KEYBOARD_END (33) before the
// manager hook (no keymap action targets them), so they are polled here and
// fed through the hook as if the engine had routed them. Home wins a tie.
void PollHomeEndKeys() {
    const bool home = acc::hotkeys::Pressed(acc::hotkeys::Action::NavHome);
    const bool end = acc::hotkeys::Pressed(acc::hotkeys::Action::NavEnd);
    if (!home && !end) return;

    void* mgr = GetGuiManager();
    if (!mgr) {
        acclog::Write("Menus.PollHomeEnd", "%s pressed but mgr=NULL; ignored",
                      home ? "Home" : "End");
        return;
    }
    s_synthesizedNav = true;
    ::OnHandleInputEvent(mgr, home ? kInputHome : kInputEnd, /*param_2=*/1);
    s_synthesizedNav = false;
}

// After all monitors, which keeps deep engine re-entry off the input hook's
// stack.
void TickPendingOps() {
    pending::Drain(GetGuiManager());
}

namespace {

// After a chain step the engine can echo a SetActive on a sibling chain
// control (the InGameOptions hit-test miss: the chain steps to Gameplay, the
// engine activates Load Game). The chain already spoke its target, so a slot
// on another entry of the bound chain is that echo. Focus on a non-chain
// control still speaks.
bool IsChainSiblingEcho(void* panel, void* control) {
    using namespace chain;
    if (g_chainPanel != panel || g_chainIndex < 0 || g_chainIndex >= g_chainCount ||
        g_chain[g_chainIndex].control == control) {
        return false;
    }
    for (int i = 0; i < g_chainCount; ++i) {
        if (g_chain[i].control != control) continue;
        acclog::Write("Announce", "chain-coherence drop: slot=%p (chain[%d]) != chain[%d]=%p",
                      control, i, g_chainIndex, g_chain[g_chainIndex].control);
        return true;
    }
    return false;
}

}  // namespace

void DrainPendingAnnounce() {
    void* panel   = s_pendingAnnouncePanel;
    void* control = s_pendingAnnounceControl;
    ClearPendingAnnounce();
    if (!control) return;

    // A panel's auto-focus on a list container is not navigation.
    const bool isListBoxContainer = IsListBox(control);
    if (detail::IsMultiRowListBox(control)) return;
    if (IsChainSiblingEcho(panel, control)) return;

    char text[detail::kAnnounceTextMax];
    if (extract::FromControl(control, text, sizeof(text), panel)) {
        SpeakIfChanged(/*channel=*/0, text);
        return;
    }
    // An empty or not yet filled list (the workbench's LB_ITEMS, id 0, spoke
    // "control 0" on open).
    if (isListBoxContainer) return;

    // A chargen class icon is readable only once the panel's OnEnterButton
    // has written class_label, a beat after the SetActive that queued this
    // (KOTOR 2 always loses that race); the focus monitor speaks it a tick
    // later.
    if (detail::IsClassSelectionIcon(panel, control)) {
        acclog::Write("Announce", "class-icon %p deferred: label cache cold", control);
        return;
    }

    // No text at all: a placeholder past the dedup, as a repeated
    // "control 11" beats a silent focus change.
    const int id = ReadControlId(control);
    char placeholder[64];
    snprintf(placeholder, sizeof(placeholder), "control %d", id);
    acc::speech::Speak(placeholder, /*interrupt=*/false);
}

void ClearPendingAnnounce() {
    s_pendingAnnouncePanel   = nullptr;
    s_pendingAnnounceControl = nullptr;
}

bool IsDrilledIntoSubScreen() { return g_drilledIntoSubScreen; }
void SetDrilledIntoSubScreen(bool drilled) { g_drilledIntoSubScreen = drilled; }

}  // namespace acc::menus

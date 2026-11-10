// The general per-tick menu monitors, which watch broad menu state and
// announce changes:
//
//   * the focus monitor — re-extracts the focused chain entry's text each
//     tick and speaks the diff (toggle flips, cycle values, sliders);
//   * the content monitor — fingerprints content-monitored panels
//     (tutorials, modal messages, transitions, the map) and speaks new
//     segments; it also announces a newly opened in-game sub-screen and
//     arms the drill into it;
//   * the dialogue reply monitor — polls the reply listbox's selection,
//     which engine arrow-key navigation changes without SetActiveControl;
//   * the inventory repair — repopulates a lazily filled item list.
//
// The monitors tied to one listbox screen (container, equip picker) live
// in menus_listbox.cpp with the specs that own their state.

#pragma once

#include "engine_panels.h"

namespace acc::menus::monitors {

// Per-tick fan-out for the monitors. Called from TickMonitors in menus.cpp,
// which core_tick::Dispatch calls.
void TickGeneralMonitors();

// Speak a control's announceable text (or "control N" placeholder),
// keeping the focus monitor's last-seen state in sync. Used by chain-step
// and Enter-on-text-only handlers in OnHandleInputEvent.
void AnnounceControl(void* control);

// An in-game sub-screen panel currently in the manager's panels[], the
// journal only when nothing else is open; nullptr when none. Used by the
// drill router to retarget the chain from the always-foreground icon strip
// to the sub-screen the user just opened.
void* FindActiveSubScreenPanel();

// True if this panel kind is one of the in-game sub-screens reached via the
// icon strip (Inventory, Map, Journal, …).
bool IsInGameSubScreenKind(acc::engine::PanelKind k);

}  // namespace acc::menus::monitors

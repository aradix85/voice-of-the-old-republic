# menus_monitors.h (43 lines)

Source: `patches/Accessibility/menus_monitors.h`

The general per-tick menu monitors, which watch broad menu state and announce changes:

* the focus monitor — re-extracts the focused chain entry's text each tick and speaks the diff (toggle flips, cycle values, sliders); * the content monitor — fingerprints content-monitored panels (tutorials, modal messages, transitions, the map) and speaks new segments; it also announces a newly opened in-game sub-screen and arms the drill into it; * the dialogue reply monitor — polls the reply listbox's selection, which engine arrow-key navigation changes without SetActiveControl; * the inventory repair — repopulates a lazily filled item list.

The monitors tied to one listbox screen (container, equip picker) live in menus_listbox.cpp with the specs that own their state.

## Declarations (in source order)

- L25 `void TickGeneralMonitors()`: Per-tick fan-out for the monitors.
- L30 `void AnnounceControl(void* control)`: Speak a control's announceable text (or "control N" placeholder), keeping the focus monitor's last-seen state in sync.
- L36 `void* FindActiveSubScreenPanel()`: An in-game sub-screen panel currently in the manager's panels[], the journal only when nothing else is open; nullptr when none.
- L40 `bool IsInGameSubScreenKind(acc::engine::PanelKind k)`: True if this panel kind is one of the in-game sub-screens reached via the icon strip (Inventory, Map, Journal, …).

# menus_monitors.h (60 lines)

Source: `patches/Accessibility/menus_monitors.h`

general per-tick monitors.

Post-Step-5 cleanup. Three monitors that watch broad menu state and announce changes per tick lift out of menus.cpp:

* MonitorFocusedControl — re-extracts the focused chain entry's text each tick and speaks the diff against the last snapshot. Catches toggle flips, cycle value changes, slider movement, and any other state visible through extract::FromControl. * MonitorPanelContents — fingerprints content-monitored panels and speaks new segments on change. Handles tutorials, dialogue text, transitions, modal messages, and the in-game sub-screen kind names. * MonitorDialogReplies — polls the dialog reply listbox's selection index on each tick (engine arrow-key navigation mutates it without firing SetActiveControl).

The three subsystem-paired monitors (MonitorContainerSelection, MonitorEquipPickerSelection, PollContainerGiveModeKey) live in menus_listbox.cpp instead, co-located with the spec entries that own the state they watch.

AnnounceControl moves with the focus monitor because it writes the monitor's last-seen state (g_focusMonitorControl / g_focusMonitorText) to keep speech in sync — they're a unit. menus.cpp's chain-step / Enter handlers in OnHandleInputEvent call AnnounceControl through this header.

Sub-screen tracking (InGameSubScreenSpec table, FindInGameSubScreenSpec, FindActiveSubScreenPanel, AnnounceNewSubScreens, g_visibleSubScreens) also moves here. menus.cpp's drill router needs FindActiveSubScreenPanel + a kind predicate, both exposed below.

## Declarations (in source order)

- L40 `void TickGeneralMonitors()`: Per-tick fan-out for the 3 general monitors.
- L45 `void AnnounceControl(void* control)`: Speak a control's announceable text (or "control N" placeholder), keeping the focus-monitor's last-seen state in sync.
- L51 `void* FindActiveSubScreenPanel()`: Find an InGame{X} sub-screen panel currently in the manager's panels[].
- L57 `bool IsInGameSubScreenKind(acc::engine::PanelKind k)`: True if this panel kind is one of the in-game sub-screens reached via the icon strip (Inventory, Map, Journal, …).

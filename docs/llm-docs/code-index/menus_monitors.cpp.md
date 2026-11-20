# menus_monitors.cpp (348 lines)

Source: `patches/Accessibility/menus_monitors.cpp`

The general per-tick menu monitors: TickGeneralMonitors, the focus monitor and AnnounceControl, which keeps the focus monitor's last-seen state in sync when it speaks. Also the cursor park the keyboard-driven listbox screens share.

## Declarations (in source order)

- L62 `bool ParkCursorToCorner(const char* tag)`
- L88 `void MonitorFocusedControl()`
- L241 `void AnnounceControl(void* control)`
- L321 `void TickGeneralMonitors()`

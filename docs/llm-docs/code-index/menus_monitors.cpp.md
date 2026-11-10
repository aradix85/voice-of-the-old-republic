# menus_monitors.cpp (356 lines)

Source: `patches/Accessibility/menus_monitors.cpp`

The general per-tick menu monitors: TickGeneralMonitors, the focus monitor and AnnounceControl, which keeps the focus monitor's last-seen state in sync when it speaks. Also the cursor park the keyboard-driven listbox screens share.

## Declarations (in source order)

- L61 `bool ParkCursorToCorner(const char* tag)`
- L87 `void MonitorFocusedControl()`
- L241 `void AnnounceControl(void* control)`
- L330 `void TickGeneralMonitors()`

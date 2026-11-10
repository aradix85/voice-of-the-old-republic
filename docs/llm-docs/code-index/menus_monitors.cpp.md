# menus_monitors.cpp (365 lines)

Source: `patches/Accessibility/menus_monitors.cpp`

general per-tick monitors.

Post-Step-5 cleanup. See menus_monitors.h for the rationale and public surface. Function bodies are unchanged from the inline originals in menus.cpp; only namespacing and linkage changed.

## Declarations (in source order)

- L65 `bool acc::menus::detail::ParkCursorToCorner(const char* tag)`
- L89 `void MonitorFocusedControl()`
- L246 `void AnnounceControl(void* control)`
- L339 `void TickGeneralMonitors()`

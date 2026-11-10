# menus_monitors_subscreens.cpp (197 lines)

Source: `patches/Accessibility/menus_monitors_subscreens.cpp`

The in-game sub-screens (equipment, inventory, map, journal and the rest of the icon strip): their table, announcing a newly opened one, and arming the drill into it.

## Declarations (in source order)

- L56 `uint32_t SpecStrref(const InGameSubScreenSpec* s)`: Per-game strref for a spec row.
- L61 `bool IsSubScreenTracked(void* p)`
- L70 `const InGameSubScreenSpec* FindSpec(PanelKind k)`
- L78 `void AnnounceNewSubScreens(void** panels, int count)`
- L170 `void* FindActiveSubScreenPanel()`
- L192 `bool IsInGameSubScreenKind(PanelKind k)`

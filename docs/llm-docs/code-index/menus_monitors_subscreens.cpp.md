# menus_monitors_subscreens.cpp (181 lines)

Source: `patches/Accessibility/menus_monitors_subscreens.cpp`

The in-game sub-screens (equipment, inventory, map, journal and the rest of the icon strip): their table, announcing a newly opened one, and arming the drill into it.

## Declarations (in source order)

- L68 `const InGameSubScreenSpec* FindSpec(PanelKind k)`
- L76 `bool IsSubScreenTracked(void* p)`
- L85 `void AnnounceSubScreen(void* p, PanelKind k, const InGameSubScreenSpec& spec)`: Speaks a newly opened sub-screen's name: the quest-items modal's live title, otherwise the game's strref, otherwise the row's literal.
- L115 `void AnnounceNewSubScreens(void** panels, int count)`
- L154 `void* FindActiveSubScreenPanel()`
- L176 `bool IsInGameSubScreenKind(PanelKind k)`

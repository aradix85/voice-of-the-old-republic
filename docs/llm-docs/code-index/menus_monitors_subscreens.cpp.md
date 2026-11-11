# menus_monitors_subscreens.cpp (179 lines)

Source: `patches/Accessibility/menus_monitors_subscreens.cpp`

The in-game sub-screens (equipment, inventory, map, journal and the rest of the icon strip): their table, announcing a newly opened one, and arming the drill into it.

## Declarations (in source order)

- L66 `const InGameSubScreenSpec* FindSpec(PanelKind k)`
- L74 `bool IsSubScreenTracked(void* p)`
- L83 `void AnnounceSubScreen(void* p, PanelKind k, const InGameSubScreenSpec& spec)`: Speaks a newly opened sub-screen's name: the quest-items modal's live title, otherwise the game's strref, otherwise the row's literal.
- L113 `void AnnounceNewSubScreens(void** panels, int count)`
- L152 `void* FindActiveSubScreenPanel()`
- L174 `bool IsInGameSubScreenKind(PanelKind k)`

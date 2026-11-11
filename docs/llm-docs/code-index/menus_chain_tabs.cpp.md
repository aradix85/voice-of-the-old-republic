# menus_chain_tabs.cpp (106 lines)

Source: `patches/Accessibility/menus_chain_tabs.cpp`

Tabbed panels: finding a panel's row of tab buttons and where a click on a tab lands.

## Declarations (in source order)

- L17 `int ComputeTabClickOffsetY(void* panel)`: The vertical spacing of the tab buttons: the click/warp offset for a tab.
- L32 `bool DetectTabsCluster(void* panel, int& outStart, int& outCount)`
- L93 `bool IsTabButton(void* control)`

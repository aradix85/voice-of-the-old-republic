# menus_chain_tabs.cpp (114 lines)

Source: `patches/Accessibility/menus_chain_tabs.cpp`

Tabbed panels: finding a panel's row of tab buttons and where a click on a tab lands.

## Declarations (in source order)

- L12 `int ComputeTabClickOffsetY(void* panel)`
- L31 `bool DetectTabsCluster(void* panel, int& outStart, int& outCount)`
- L101 `bool IsTabButton(void* control)`

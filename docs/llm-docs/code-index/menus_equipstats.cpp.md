# menus_equipstats.cpp (138 lines)

Source: `patches/Accessibility/menus_equipstats.cpp`

virtual stat-row anchors for the Equip panel. See menus_equipstats.h for design summary.

## Declarations (in source order)

- L63 `const EquipStatRowSpec* FindSpecForControl(void* panel, void* labelControl)`
- L82 `bool IsEquipStatRowAnchor(void* panel, void* labelControl)`
- L86 `void ForEachEquipStatRowAnchor(void* panel, bool (*callback)(void* labelControl, int sortCy, void* userData), void* userData)`
- L103 `bool ExtractEquipStatRow(void* panel, void* labelControl, char* outBuf, size_t bufSize)`

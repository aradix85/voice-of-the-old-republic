# menus_chargen_feats.cpp (98 lines)

Source: `patches/Accessibility/menus_chargen_feats.cpp`

The feat chart of character generation and level-up: what menus_skillflow_grid needs to know about CSWGuiFeatsCharGen.

## Declarations (in source order)

- L23 `void* At(void* panel, size_t offset)`
- L27 `void* Chart(void* panel)`
- L28 `void* NameLabel(void* panel)`
- L29 `void* DescriptionListBox(void* panel)`
- L32 `bool ReadRows(void* panel, void** rows, int* count)`: The rows are the chart's own array.
- L46 `void OnEnterFeat(void* panel, unsigned long featId)`: Refreshes the name label, the description and the Add / Remove button.
- L50 `void OnFeatPicked(void* panel, unsigned long featId)`
- L87 `bool IsChargenFeatsPanel(void* panel)`
- L93 `bool HandleInput(void* panel, int code, int value, int& outRv)`

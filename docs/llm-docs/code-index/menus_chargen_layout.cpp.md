# menus_chargen_layout.cpp (229 lines)

Source: `patches/Accessibility/menus_chargen_layout.cpp`

See menus_chargen_layout.h for purpose.

## Declarations (in source order)

- L21 `int RowPitchFromButtonExtents(void* panel, size_t buttonsArrayOffset)`
- L45 `bool IsPanel(const PanelDesc& d, void* panel)`
- L49 `int IndexFromButton(const PanelDesc& d, void* panel, void* control)`
- L77 `int RowPitchForCursorWarp(const PanelDesc& d, void* panel, void* control)`: KOTOR 1 ONLY, same shape as the class-icon column shim in menus_chain.cpp: KOTOR 1's abchrgen/skchrgen hit-test resolves one row ABOVE the value button's own extent, so the warp has to aim a full row-pitch low or the ...
- L83 `void SyncSelectedFromChainFocus(const PanelDesc& d)`
- L112 `void CaptureLabels(const PanelDesc& d, void* panel)`
- L137 `void LogBindingWord(const PanelDesc& d, void* panel, const char* when)`: Diagnostic: dump the panel's binding word and its neighbours.
- L168 `bool AnnounceDescription(const PanelDesc& d, void* panel, void* control)`
- L220 `bool IsDescriptionListbox(const PanelDesc& d, void* listBox)`

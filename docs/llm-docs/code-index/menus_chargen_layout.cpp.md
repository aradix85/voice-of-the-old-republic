# menus_chargen_layout.cpp (269 lines)

Source: `patches/Accessibility/menus_chargen_layout.cpp`

See menus_chargen_layout.h for purpose.

## Declarations (in source order)

- L21 `int RowPitchFromButtonExtents(void* panel, size_t buttonsArrayOffset)`
- L45 `bool IsPanel(const PanelDesc& d, void* panel)`
- L56 `int IndexFromButton(const PanelDesc& d, void* panel, void* control)`
- L84 `int RowPitchForCursorWarp(const PanelDesc& d, void* panel, void* control)`: KOTOR 1 ONLY, same shape as the class-icon column shim in menus_chain.cpp: KOTOR 1's abchrgen/skchrgen hit-test resolves one row ABOVE the value button's own extent, so the warp has to aim a full row-pitch low or the ...
- L90 `void SyncSelectedFromChainFocus(const PanelDesc& d)`
- L119 `void CaptureLabels(const PanelDesc& d, void* panel)`
- L144 `void LogBindingWord(const PanelDesc& d, void* panel, const char* when)`: Diagnostic: dump the panel's binding word and its neighbours.
- L175 `bool IsBindingReady(const PanelDesc& d, void* panel)`
- L197 `bool AnnounceDescription(const PanelDesc& d, void* panel, void* control)`
- L260 `bool IsDescriptionListbox(const PanelDesc& d, void* listBox)`

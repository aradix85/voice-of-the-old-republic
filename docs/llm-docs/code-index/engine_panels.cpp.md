# engine_panels.cpp (328 lines)

Source: `patches/Accessibility/engine_panels.cpp`

Panel identity: the CGuiInGame slot table, IdentifyPanel and the probe that logs a panel nothing recognises.

## Declarations (in source order)

- L136 `void LogIdentifiedOnce(void* panel, PanelKind kind)`
- L160 `bool IsVtableAlreadyDumped(uintptr_t vt)`
- L167 `void RememberDumpedVtable(uintptr_t vt)`
- L177 `void LogUnknownPanelDiagnostics(void* panel)`
- L237 `int SlotTableLookup(void* panel)`: The CGuiInGame slot walk: the matching table index, or -1.
- L257 `const char* PanelKindName(PanelKind k)`
- L265 `PanelKind IdentifyPanel(void* panel)`
- L279 `bool IsPanelKindInGameMenu(void* panel)`
- L283 `void* FindPanelByKind(PanelKind kind)`
- L295 `bool IsMainMenuOptionsSubScreen(PanelKind k)`
- L312 `bool IsModalPopupPanel(PanelKind k)`

# engine_panels.cpp (326 lines)

Source: `patches/Accessibility/engine_panels.cpp`

Panel identity: the CGuiInGame slot table, IdentifyPanel and the probe that logs a panel nothing recognises.

## Declarations (in source order)

- L136 `void LogIdentifiedOnce(void* panel, PanelKind kind)`
- L160 `bool IsVtableAlreadyDumped(uintptr_t vt)`
- L167 `void RememberDumpedVtable(uintptr_t vt)`
- L177 `void LogUnknownPanelDiagnostics(void* panel)`
- L235 `int SlotTableLookup(void* panel)`: The CGuiInGame slot walk: the matching table index, or -1.
- L255 `const char* PanelKindName(PanelKind k)`
- L263 `PanelKind IdentifyPanel(void* panel)`
- L277 `bool IsPanelKindInGameMenu(void* panel)`
- L281 `void* FindPanelByKind(PanelKind kind)`
- L293 `bool IsMainMenuOptionsSubScreen(PanelKind k)`
- L310 `bool IsModalPopupPanel(PanelKind k)`

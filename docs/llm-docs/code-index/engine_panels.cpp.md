# engine_panels.cpp (320 lines)

Source: `patches/Accessibility/engine_panels.cpp`

Panel identity: the CGuiInGame slot table, IdentifyPanel and the probe that logs a panel nothing recognises.

## Declarations (in source order)

- L136 `void LogIdentifiedOnce(void* panel, PanelKind kind)`
- L160 `bool IsVtableAlreadyDumped(uintptr_t vt)`
- L167 `void RememberDumpedVtable(uintptr_t vt)`
- L177 `void LogUnknownPanelDiagnostics(void* panel)`
- L229 `int SlotTableLookup(void* panel)`: The CGuiInGame slot walk: the matching table index, or -1.
- L249 `const char* PanelKindName(PanelKind k)`
- L257 `PanelKind IdentifyPanel(void* panel)`
- L271 `bool IsPanelKindInGameMenu(void* panel)`
- L275 `void* FindPanelByKind(PanelKind kind)`
- L287 `bool IsMainMenuOptionsSubScreen(PanelKind k)`
- L304 `bool IsModalPopupPanel(PanelKind k)`

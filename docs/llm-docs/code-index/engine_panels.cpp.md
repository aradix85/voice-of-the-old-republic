# engine_panels.cpp (334 lines)

Source: `patches/Accessibility/engine_panels.cpp`

Panel identity: the CGuiInGame slot table, IdentifyPanel and the probe that logs a panel nothing recognises.

## Declarations (in source order)

- L136 `void LogIdentifiedOnce(void* panel, PanelKind kind)`
- L160 `bool IsVtableAlreadyDumped(uintptr_t vt)`
- L167 `void RememberDumpedVtable(uintptr_t vt)`
- L177 `void LogUnknownPanelDiagnostics(void* panel)`
- L243 `int SlotTableLookup(void* panel)`: The CGuiInGame slot walk: the matching table index, or -1.
- L263 `const char* PanelKindName(PanelKind k)`
- L271 `PanelKind IdentifyPanel(void* panel)`
- L285 `bool IsPanelKindInGameMenu(void* panel)`
- L289 `void* FindPanelByKind(PanelKind kind)`
- L301 `bool IsMainMenuOptionsSubScreen(PanelKind k)`
- L318 `bool IsModalPopupPanel(PanelKind k)`

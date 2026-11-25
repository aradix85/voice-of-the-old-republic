# engine_panels.cpp (332 lines)

Source: `patches/Accessibility/engine_panels.cpp`

Panel identity: the CGuiInGame slot table, IdentifyPanel and the probe that logs a panel nothing recognises.

## Declarations (in source order)

- L148 `void LogIdentifiedOnce(void* panel, PanelKind kind)`
- L172 `bool IsVtableAlreadyDumped(uintptr_t vt)`
- L179 `void RememberDumpedVtable(uintptr_t vt)`
- L189 `void LogUnknownPanelDiagnostics(void* panel)`
- L241 `int SlotTableLookup(void* panel)`: The CGuiInGame slot walk: the matching table index, or -1.
- L261 `const char* PanelKindName(PanelKind k)`
- L269 `PanelKind IdentifyPanel(void* panel)`
- L283 `bool IsPanelKindInGameMenu(void* panel)`
- L287 `void* FindPanelByKind(PanelKind kind)`
- L299 `bool IsMainMenuOptionsSubScreen(PanelKind k)`
- L316 `bool IsModalPopupPanel(PanelKind k)`

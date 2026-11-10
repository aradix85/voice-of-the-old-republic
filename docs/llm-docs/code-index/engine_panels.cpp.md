# engine_panels.cpp (475 lines)

Source: `patches/Accessibility/engine_panels.cpp`

(From engine_panels.h.) In-game panel identity registry. Classifies CSWGuiPanel pointers against named CGuiInGame slots so menu code can branch on semantic kind instead of layout.

Chain: *(CAppManager**)0x7A39FC → CAppManager → +0x4 CClientExoApp → +0x4 Internal → +0x40 CGuiInGame*. Re-resolved on every call so module transitions don't strand stale pointers.

## Declarations (in source order)

- L193 `bool IsVtableAlreadyDumped(uintptr_t vt)`
- L200 `void RememberDumpedVtable(uintptr_t vt)`
- L210 `void LogUnknownPanelDiagnostics(void* panel)`
- L287 `int SlotTableLookup(void* panel)`: The CGuiInGame slot walk, hoisted out of IdentifyPanel so the __try owns no unwinding objects (C2712) and returns the matching table index, or -1.
- L307 `const char* PanelKindName(PanelKind k)`
- L315 `PanelKind IdentifyPanel(void* panel)`
- L426 `bool IsPanelKindInGameMenu(void* panel)`
- L430 `void* FindPanelByKind(PanelKind kind)`
- L442 `bool IsMainMenuOptionsSubScreen(PanelKind k)`
- L459 `bool IsModalPopupPanel(PanelKind k)`

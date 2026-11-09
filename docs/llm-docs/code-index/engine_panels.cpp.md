# engine_panels.cpp (905 lines)

Source: `patches/Accessibility/engine_panels.cpp`

(From engine_panels.h.) In-game panel identity registry. Classifies CSWGuiPanel pointers against named CGuiInGame slots so menu code can branch on semantic kind instead of layout.

Chain: *(CAppManager**)0x7A39FC → CAppManager → +0x4 CClientExoApp → +0x4 Internal → +0x40 CGuiInGame*. Re-resolved on every call so module transitions don't strand stale pointers.

## Declarations (in source order)

- L21 `bool HasVtable(void* obj, uintptr_t expected)`: Declared in engine_panels.h.
- L44 `bool IsSaveLoadStructural(void* panel)`
- L60 `bool IsWorkbenchUpgradeStructural(void* panel)`
- L73 `bool IsInGameMessagesStructural(void* panel)`
- L87 `bool IsWorkbenchItemsStructural(void* panel)`
- L108 `bool IsWorkbenchSelectStructural(void* panel)`
- L122 `bool IsWorkbenchCreateItemStructural(void* panel)`
- L133 `bool IsWorkbenchCreateMedicalStructural(void* panel)`
- L150 `bool IsLevelUpStructural(void* panel)`
- L165 `bool IsCharGenStructural(void* panel)`
- L185 `bool IsMainMenuOptionsStructural(void* panel)`
- L197 `bool IsMainMenuStructural(void* panel)`
- L207 `bool IsPazaakStartStructural(void* panel)`
- L217 `bool IsPazaakWagerStructural(void* panel)`
- L232 `bool IsQuestItemStructural(void* panel)`
- L246 `bool IsScriptSelectStructural(void* panel)`
- L253 `bool IsGamepadQuickMenuStructural(void* panel)`: KOTOR 2 gamepad Quick Menu — vtable equality, nothing structural.
- L274 `bool IsPowersLevelUpStructural(void* panel)`: CSWGuiPowersLevelUp picker (pwrlvlup.gui).
- L319 `PanelKind IdentifyOptionsSubScreen(void* panel)`: Returns the specific sub-screen kind, or Unknown if `panel`'s vtable isn't one of the nine.
- L341 `bool LookupUpgradeSlotType(int slotCustomValue, int category, int* outUpgradeType, uint32_t* outStrRef)`: See the header for why the index lives here rather than at the call sites.
- L531 `const char* PanelKindName(PanelKind k)`
- L539 `void* ResolveGuiInGame()`
- L552 `void* ResolveMainInterface()`
- L564 `bool ReadDialogReplyText(int replyIndex, char* outBuf, size_t bufSize)`
- L589 `int ReadDialogReplyCount()`
- L628 `bool IsVtableAlreadyDumped(uintptr_t vt)`
- L635 `void RememberDumpedVtable(uintptr_t vt)`
- L645 `void LogUnknownPanelDiagnostics(void* panel)`
- L722 `int SlotTableLookup(void* panel)`: The CGuiInGame slot walk, hoisted out of IdentifyPanel so the __try owns no unwinding objects (C2712) and returns the matching table index, or -1.
- L742 `PanelKind IdentifyPanel(void* panel)`
- L856 `bool IsPanelKindInGameMenu(void* panel)`
- L860 `void* FindPanelByKind(PanelKind kind)`
- L872 `bool IsMainMenuOptionsSubScreen(PanelKind k)`
- L889 `bool IsModalPopupPanel(PanelKind k)`

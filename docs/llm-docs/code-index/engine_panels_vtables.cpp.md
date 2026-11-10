# engine_panels_vtables.cpp (313 lines)

Source: `patches/Accessibility/engine_panels_vtables.cpp`

The heap-allocated panels with no CGuiInGame slot, identified by their vtable.

## Declarations (in source order)

- L178 `bool HasVtable(void* obj, uintptr_t expected)`: Declared in engine_panels.h.
- L200 `bool IsSaveLoadStructural(void* panel)`
- L204 `bool IsWorkbenchUpgradeStructural(void* panel)`
- L208 `bool IsInGameMessagesStructural(void* panel)`
- L213 `bool IsWorkbenchItemsStructural(void* panel)`
- L217 `bool IsWorkbenchSelectStructural(void* panel)`
- L221 `bool IsWorkbenchCreateItemStructural(void* panel)`
- L232 `bool IsWorkbenchCreateMedicalStructural(void* panel)`
- L243 `bool IsLevelUpStructural(void* panel)`
- L247 `bool IsCharGenStructural(void* panel)`
- L258 `bool IsMainMenuOptionsStructural(void* panel)`
- L262 `bool IsMainMenuStructural(void* panel)`
- L266 `bool IsPazaakStartStructural(void* panel)`
- L270 `bool IsPazaakWagerStructural(void* panel)`
- L274 `bool IsQuestItemStructural(void* panel)`
- L278 `bool IsScriptSelectStructural(void* panel)`
- L292 `bool IsPowersLevelUpStructural(void* panel)`: CSWGuiPowersLevelUp picker (pwrlvlup.gui).
- L300 `PanelKind IdentifyOptionsSubScreen(void* panel)`: Returns the specific sub-screen kind, or Unknown if `panel`'s vtable isn't one of the nine.

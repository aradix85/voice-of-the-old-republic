# engine_panels_internal.h (66 lines)

Source: `patches/Accessibility/engine_panels_internal.h`

Internal to the engine_panels module (engine_panels*.cpp); no other file includes it. The public interface is engine_panels.h. Here: what the files share.

## Declarations (in source order)

- L42 `void* ResolveGuiInGame()`: Walk that chain and return the CGuiInGame*.
- L47 `bool IsSaveLoadStructural(void* panel)`: engine_panels_vtables.cpp
- L48 `bool IsWorkbenchUpgradeStructural(void* panel)`
- L49 `bool IsInGameMessagesStructural(void* panel)`
- L50 `bool IsWorkbenchItemsStructural(void* panel)`
- L51 `bool IsWorkbenchSelectStructural(void* panel)`
- L52 `bool IsWorkbenchCreateItemStructural(void* panel)`
- L53 `bool IsWorkbenchCreateMedicalStructural(void* panel)`
- L54 `bool IsLevelUpStructural(void* panel)`
- L55 `bool IsCharGenStructural(void* panel)`
- L56 `bool IsMainMenuOptionsStructural(void* panel)`
- L57 `bool IsMainMenuStructural(void* panel)`
- L58 `bool IsPazaakStartStructural(void* panel)`
- L59 `bool IsPazaakWagerStructural(void* panel)`
- L60 `bool IsQuestItemStructural(void* panel)`
- L61 `bool IsScriptSelectStructural(void* panel)`
- L62 `bool IsPowersLevelUpStructural(void* panel)`
- L63 `PanelKind IdentifyOptionsSubScreen(void* panel)`

Also 2 constants or variables at namespace scope.

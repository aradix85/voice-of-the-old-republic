# engine_panels_state.cpp (430 lines)

Source: `patches/Accessibility/engine_panels_state.cpp`

foreground / UI-blocking model and GUI input-class plumbing.

Split out of engine_panels.cpp by the Phase-1 structure pass (refactoring candidate 6). engine_panels.cpp stays the panel-identity registry - structural detectors, the PanelKind offset table, IdentifyPanel and the PanelKind-based classification predicates. This file owns the runtime-state questions layered on top of that registry:

* is a dialog / bark bubble / map / level-up panel up right now * show / hide the in-game GUI and close back to the world * get / set the GUI input class * IsForegroundUiBlocking - the mod-wide "is the UI eating input" gate

Declarations stay in engine_panels.h, so all 39 includers are unaffected.

## Declarations (in source order)

- L31 `bool HasActiveDialogPanel()`
- L58 `bool HasActiveBarkBubble()`
- L72 `bool CallPrevSWInGameGui()`
- L105 `bool CallHideSWInGameGui(int param_1)`
- L140 `bool SetGlobalDialogState(int state)`
- L173 `bool CloseInGameMenuToWorld()`
- L215 `int GetInputClass()`: Read the live input_class (CClientExoAppInternal +0x9c).
- L238 `bool SetGuiInputClass(int klass)`: Set the client input class via the engine's own setter.
- L265 `bool HasActiveMapPanel(void** outPanel)`
- L281 `bool HasActiveLevelUpPanel()`
- L293 `bool IsInGameOptionsSubScreen(void* panel)`
- L308 `bool HasActiveSubScreen()`
- L337 `bool IsForegroundUiBlocking(UiBlockState* outState)`
- L412 `bool IsWorldLevelForegroundKind(PanelKind k)`

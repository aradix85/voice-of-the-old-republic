# engine_panels_state.cpp (384 lines)

Source: `patches/Accessibility/engine_panels_state.cpp`

foreground / UI-blocking model and GUI input-class plumbing.

Split out of engine_panels.cpp by the Phase-1 structure pass (refactoring candidate 6). engine_panels.cpp stays the panel-identity registry - structural detectors, the PanelKind offset table, IdentifyPanel and the PanelKind-based classification predicates. This file owns the runtime-state questions layered on top of that registry:

* is a dialog / bark bubble / map / level-up panel up right now * show / hide the in-game GUI and close back to the world * get / set the GUI input class * IsForegroundUiBlocking - the mod-wide "is the UI eating input" gate

Declarations stay in engine_panels.h, so all 39 includers are unaffected.

## Declarations (in source order)

- L32 `bool HasActiveDialogPanel()`
- L59 `bool HasActiveBarkBubble()`
- L73 `bool CallPrevSWInGameGui()`
- L111 `bool SetGlobalDialogState(int state)`
- L144 `bool CloseInGameMenuToWorld()`
- L180 `int GetInputClass()`: Read the live input_class (CClientExoAppInternal +0x9c).
- L196 `bool SetGuiInputClass(int klass)`: Set the client input class via the engine's own setter.
- L213 `bool HasActiveMapPanel(void** outPanel)`
- L229 `bool HasActiveLevelUpPanel()`
- L241 `bool IsInGameOptionsSubScreen(void* panel)`
- L256 `bool HasActiveSubScreen()`
- L285 `bool IsForegroundUiBlocking(UiBlockState* outState)`
- L366 `bool IsWorldLevelForegroundKind(PanelKind k)`

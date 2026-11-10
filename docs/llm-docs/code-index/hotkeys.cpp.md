# hotkeys.cpp (208 lines)

Source: `patches/Accessibility/hotkeys.cpp`

central hotkey registry implementation. See hotkeys.h for the design rationale + API contract.

## Declarations (in source order)

- L38 `bool IsDownVk(int vk)`
- L48 `uint32_t ReadModifiers()`: Compute the current modifier mask from OS state.
- L68 `bool BindingKeyDown(const Binding& b)`: Match a binding against the current keyboard state.
- L73 `bool BindingMatches(const Binding& b, uint32_t mods)`
- L86 `void BeginTick()`
- L96 `void EndTick()`
- L106 `bool IsForegroundGame()`
- L114 `bool Pressed(Action a)`
- L125 `bool Held(Action a)`
- L132 `void Consume(Action a)`
- L141 `void ClaimRisingEdge(Action a)`
- L148 `bool ShiftHeld()`
- L152 `bool CtrlHeld()`
- L156 `uint32_t CurrentModifiers()`
- L160 `bool ModifiedComboOwns(int vk)`
- L188 `Binding Get(Action a)`
- L195 `void Set(Action a, Binding b)`

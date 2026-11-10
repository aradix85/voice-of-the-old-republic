# hotkeys.cpp (158 lines)

Source: `patches/Accessibility/hotkeys.cpp`

The hotkey registry: the live bindings and the per-tick edge detection. See hotkeys.h for the API contract.

## Declarations (in source order)

- L34 `bool IsDownVk(int vk)`
- L38 `bool AnyDown(int a, int b, int c)`
- L44 `uint32_t ReadModifiers()`: AltGr (right Alt) counts as Alt too, but has its own bit for the bindings that tell the two apart.
- L53 `bool BindingKeyDown(const Binding& b)`
- L58 `int Index(Action a)`: The index of `a` once the defaults exist, or -1 when out of range.
- L66 `void BeginTick()`
- L76 `void EndTick()`
- L84 `bool IsForegroundGame()`
- L92 `bool Pressed(Action a)`
- L100 `bool Held(Action a)`
- L106 `void Consume(Action a)`: Pressed() stays false for the rest of this tick.
- L111 `void ClaimRisingEdge(Action a)`
- L116 `bool ShiftHeld()`
- L120 `bool CtrlHeld()`
- L124 `uint32_t CurrentModifiers()`
- L128 `bool ModifiedComboOwns(int vk)`
- L142 `Binding Get(Action a)`
- L147 `void Set(Action a, Binding b)`

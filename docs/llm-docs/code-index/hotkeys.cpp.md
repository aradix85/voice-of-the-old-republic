# hotkeys.cpp (158 lines)

Source: `patches/Accessibility/hotkeys.cpp`

The hotkey registry: the live bindings and the per-tick edge detection. See hotkeys.h for the API contract.

## Declarations (in source order)

- L34 `bool AnyDown(int a, int b, int c)`
- L40 `uint32_t ReadModifiers()`: AltGr (right Alt) counts as Alt too, but has its own bit for the bindings that tell the two apart.
- L49 `bool BindingKeyDown(const Binding& b)`
- L54 `int Index(Action a)`: The index of `a` once the defaults exist, or -1 when out of range.
- L62 `void BeginTick()`
- L72 `void EndTick()`
- L80 `bool IsForegroundGame()`
- L88 `bool Pressed(Action a)`
- L96 `bool Held(Action a)`
- L102 `void Consume(Action a)`: Pressed() stays false for the rest of this tick.
- L107 `void ClaimRisingEdge(Action a)`
- L112 `bool KeyDown(int vk)`
- L116 `bool ShiftHeld()`
- L120 `bool CtrlHeld()`
- L124 `uint32_t CurrentModifiers()`
- L128 `bool ModifiedComboOwns(int vk)`
- L142 `Binding Get(Action a)`
- L147 `void Set(Action a, Binding b)`

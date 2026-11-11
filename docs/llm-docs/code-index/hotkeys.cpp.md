# hotkeys.cpp (175 lines)

Source: `patches/Accessibility/hotkeys.cpp`

The hotkey registry: the live bindings and the per-tick edge detection. See hotkeys.h for the API contract.

## Declarations (in source order)

- L37 `bool AnyDown(int a, int b, int c)`
- L43 `uint32_t ReadModifiers()`: AltGr (right Alt) counts as Alt too, but has its own bit for the bindings that tell the two apart.
- L52 `bool BindingKeyDown(const Binding& b)`
- L57 `int Index(Action a)`: The index of `a` once the defaults exist, or -1 when out of range.
- L65 `void BeginTick()`
- L81 `void ChordPressSwallowed(int vk)`
- L85 `bool TakeChordPress(int vk)`
- L89 `void EndTick()`
- L97 `bool IsForegroundGame()`
- L105 `bool Pressed(Action a)`
- L113 `bool Held(Action a)`
- L119 `void Consume(Action a)`: Pressed() stays false for the rest of this tick.
- L124 `void ClaimRisingEdge(Action a)`
- L129 `bool KeyDown(int vk)`
- L133 `bool ShiftHeld()`
- L137 `bool CtrlHeld()`
- L141 `uint32_t CurrentModifiers()`
- L145 `bool ModifiedComboOwns(int vk)`
- L159 `Binding Get(Action a)`
- L164 `void Set(Action a, Binding b)`

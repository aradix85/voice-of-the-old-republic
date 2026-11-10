# hotkeys.cpp (857 lines)

Source: `patches/Accessibility/hotkeys.cpp`

central hotkey registry implementation. See hotkeys.h for the design rationale + API contract.

## Declarations (in source order)

- L154 `bool IsDownVk(int vk)`
- L164 `uint32_t ReadModifiers()`: Compute the current modifier mask from OS state.
- L184 `bool BindingKeyDown(const Binding& b)`: Match a binding against the current keyboard state.
- L189 `bool BindingMatches(const Binding& b, uint32_t mods)`
- L208 `void InitDefaults()`
- L488 `void BindKey(Action a, char* out, size_t cap)`
- L492 `void SerializeBinding(const Binding& b, char* out, size_t cap)`
- L499 `bool ParseBinding(const char* s, Binding& out)`: Parse "vk,altVk,req,forbid"; returns true on a full 4-field parse.
- L517 `void EnsureOverridesLoaded()`: Layer persisted user rebinds over the defaults.
- L545 `void BeginTick()`
- L555 `void EndTick()`
- L565 `bool IsForegroundGame()`
- L573 `bool Pressed(Action a)`
- L588 `void PadPress(Action a)`
- L596 `bool Held(Action a)`
- L603 `void Consume(Action a)`
- L612 `void ClaimRisingEdge(Action a)`
- L619 `bool ShiftHeld()`
- L623 `bool CtrlHeld()`
- L627 `bool AltHeld()`
- L631 `bool AltGrHeld()`
- L635 `uint32_t CurrentModifiers()`
- L639 `bool ModifiedComboOwns(int vk)`
- L667 `Binding Get(Action a)`
- L674 `void Set(Action a, Binding b)`
- L686 `Binding GetDefault(Action a)`
- L693 `void SetUserBinding(Action a, Binding b)`
- L704 `void ResetUserBindings()`
- L720 `Action FindConflict(Action self, int vk, uint32_t mods)`
- L740 `bool IsUserRebindable(Action a)`
- L758 `const char* Name(Action a)`
- L764 `const char* VkLabel(int vk)`
- L828 `const char* Describe(Action a)`

# hotkeys.cpp (826 lines)

Source: `patches/Accessibility/hotkeys.cpp`

central hotkey registry implementation. See hotkeys.h for the design rationale + API contract.

## Declarations (in source order)

- L150 `bool IsDownVk(int vk)`
- L160 `uint32_t ReadModifiers()`: Compute the current modifier mask from OS state.
- L180 `bool BindingKeyDown(const Binding& b)`: Match a binding against the current keyboard state.
- L185 `bool BindingMatches(const Binding& b, uint32_t mods)`
- L204 `void InitDefaults()`
- L484 `void BindKey(Action a, char* out, size_t cap)`
- L488 `void SerializeBinding(const Binding& b, char* out, size_t cap)`
- L495 `bool ParseBinding(const char* s, Binding& out)`: Parse "vk,altVk,req,forbid"; returns true on a full 4-field parse.
- L513 `void EnsureOverridesLoaded()`: Layer persisted user rebinds over the defaults.
- L541 `void BeginTick()`
- L551 `void EndTick()`
- L561 `bool IsForegroundGame()`
- L569 `bool Pressed(Action a)`
- L580 `bool Held(Action a)`
- L587 `void Consume(Action a)`
- L596 `void ClaimRisingEdge(Action a)`
- L603 `bool ShiftHeld()`
- L607 `bool CtrlHeld()`
- L611 `uint32_t CurrentModifiers()`
- L615 `bool ModifiedComboOwns(int vk)`
- L643 `Binding Get(Action a)`
- L650 `void Set(Action a, Binding b)`
- L662 `void SetUserBinding(Action a, Binding b)`
- L673 `void ResetUserBindings()`
- L689 `Action FindConflict(Action self, int vk, uint32_t mods)`
- L709 `bool IsUserRebindable(Action a)`
- L727 `const char* Name(Action a)`
- L733 `const char* VkLabel(int vk)`
- L797 `const char* Describe(Action a)`

# hotkeys.cpp (822 lines)

Source: `patches/Accessibility/hotkeys.cpp`

central hotkey registry implementation. See hotkeys.h for the design rationale + API contract.

## Declarations (in source order)

- L150 `bool IsDownVk(int vk)`
- L160 `uint32_t ReadModifiers()`: Compute the current modifier mask from OS state.
- L180 `bool BindingKeyDown(const Binding& b)`: Match a binding against the current keyboard state.
- L185 `bool BindingMatches(const Binding& b, uint32_t mods)`
- L204 `void InitDefaults()`
- L480 `void BindKey(Action a, char* out, size_t cap)`
- L484 `void SerializeBinding(const Binding& b, char* out, size_t cap)`
- L491 `bool ParseBinding(const char* s, Binding& out)`: Parse "vk,altVk,req,forbid"; returns true on a full 4-field parse.
- L509 `void EnsureOverridesLoaded()`: Layer persisted user rebinds over the defaults.
- L537 `void BeginTick()`
- L547 `void EndTick()`
- L557 `bool IsForegroundGame()`
- L565 `bool Pressed(Action a)`
- L576 `bool Held(Action a)`
- L583 `void Consume(Action a)`
- L592 `void ClaimRisingEdge(Action a)`
- L599 `bool ShiftHeld()`
- L603 `bool CtrlHeld()`
- L607 `uint32_t CurrentModifiers()`
- L611 `bool ModifiedComboOwns(int vk)`
- L639 `Binding Get(Action a)`
- L646 `void Set(Action a, Binding b)`
- L658 `void SetUserBinding(Action a, Binding b)`
- L669 `void ResetUserBindings()`
- L685 `Action FindConflict(Action self, int vk, uint32_t mods)`
- L705 `bool IsUserRebindable(Action a)`
- L723 `const char* Name(Action a)`
- L729 `const char* VkLabel(int vk)`
- L793 `const char* Describe(Action a)`

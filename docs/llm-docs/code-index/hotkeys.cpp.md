# hotkeys.cpp (858 lines)

Source: `patches/Accessibility/hotkeys.cpp`

central hotkey registry implementation. See hotkeys.h for the design rationale + API contract.

## Declarations (in source order)

- L154 `bool IsDownVk(int vk)`
- L164 `uint32_t ReadModifiers()`: Compute the current modifier mask from OS state.
- L184 `bool BindingKeyDown(const Binding& b)`: Match a binding against the current keyboard state.
- L189 `bool BindingMatches(const Binding& b, uint32_t mods)`
- L208 `void InitDefaults()`
- L489 `void BindKey(Action a, char* out, size_t cap)`
- L493 `void SerializeBinding(const Binding& b, char* out, size_t cap)`
- L500 `bool ParseBinding(const char* s, Binding& out)`: Parse "vk,altVk,req,forbid"; returns true on a full 4-field parse.
- L518 `void EnsureOverridesLoaded()`: Layer persisted user rebinds over the defaults.
- L546 `void BeginTick()`
- L556 `void EndTick()`
- L566 `bool IsForegroundGame()`
- L574 `bool Pressed(Action a)`
- L589 `void PadPress(Action a)`
- L597 `bool Held(Action a)`
- L604 `void Consume(Action a)`
- L613 `void ClaimRisingEdge(Action a)`
- L620 `bool ShiftHeld()`
- L624 `bool CtrlHeld()`
- L628 `bool AltHeld()`
- L632 `bool AltGrHeld()`
- L636 `uint32_t CurrentModifiers()`
- L640 `bool ModifiedComboOwns(int vk)`
- L668 `Binding Get(Action a)`
- L675 `void Set(Action a, Binding b)`
- L687 `Binding GetDefault(Action a)`
- L694 `void SetUserBinding(Action a, Binding b)`
- L705 `void ResetUserBindings()`
- L721 `Action FindConflict(Action self, int vk, uint32_t mods)`
- L741 `bool IsUserRebindable(Action a)`
- L759 `const char* Name(Action a)`
- L765 `const char* VkLabel(int vk)`
- L829 `const char* Describe(Action a)`

# hotkeys.cpp (868 lines)

Source: `patches/Accessibility/hotkeys.cpp`

central hotkey registry implementation. See hotkeys.h for the design rationale + API contract.

## Declarations (in source order)

- L156 `bool IsDownVk(int vk)`
- L166 `uint32_t ReadModifiers()`: Compute the current modifier mask from OS state.
- L186 `bool BindingKeyDown(const Binding& b)`: Match a binding against the current keyboard state.
- L191 `bool BindingMatches(const Binding& b, uint32_t mods)`
- L210 `void InitDefaults()`
- L497 `void BindKey(Action a, char* out, size_t cap)`
- L501 `void SerializeBinding(const Binding& b, char* out, size_t cap)`
- L508 `bool ParseBinding(const char* s, Binding& out)`: Parse "vk,altVk,req,forbid"; returns true on a full 4-field parse.
- L526 `void EnsureOverridesLoaded()`: Layer persisted user rebinds over the defaults.
- L554 `void BeginTick()`
- L564 `void EndTick()`
- L574 `bool IsForegroundGame()`
- L582 `bool Pressed(Action a)`
- L597 `void PadPress(Action a)`
- L605 `bool Held(Action a)`
- L612 `void Consume(Action a)`
- L621 `void ClaimRisingEdge(Action a)`
- L628 `bool ShiftHeld()`
- L632 `bool CtrlHeld()`
- L636 `bool AltHeld()`
- L640 `bool AltGrHeld()`
- L644 `uint32_t CurrentModifiers()`
- L648 `bool ModifiedComboOwns(int vk)`
- L676 `Binding Get(Action a)`
- L683 `void Set(Action a, Binding b)`
- L695 `Binding GetDefault(Action a)`
- L702 `void SetUserBinding(Action a, Binding b)`
- L713 `void ResetUserBindings()`
- L729 `Action FindConflict(Action self, int vk, uint32_t mods)`
- L749 `bool IsUserRebindable(Action a)`
- L769 `const char* Name(Action a)`
- L775 `const char* VkLabel(int vk)`
- L839 `const char* Describe(Action a)`

# hotkeys_user.cpp (136 lines)

Source: `patches/Accessibility/hotkeys_user.cpp`

User rebinding: the overrides in acc_settings.ini, conflicts and which actions may be rebound.

## Declarations (in source order)

- L17 `void BindKey(Action a, char* out, size_t cap)`
- L21 `void SerializeBinding(const Binding& b, char* out, size_t cap)`
- L28 `bool ParseBinding(const char* s, Binding& out)`: Parse "vk,altVk,req,forbid"; returns true on a full 4-field parse.
- L48 `void EnsureOverridesLoaded()`: Layer persisted user rebinds over the defaults.
- L72 `void SetUserBinding(Action a, Binding b)`
- L83 `void ResetUserBindings()`
- L99 `Action FindConflict(Action self, int vk, uint32_t mods)`
- L119 `bool IsUserRebindable(Action a)`

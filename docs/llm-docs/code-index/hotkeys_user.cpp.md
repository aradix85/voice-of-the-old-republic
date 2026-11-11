# hotkeys_user.cpp (101 lines)

Source: `patches/Accessibility/hotkeys_user.cpp`

User rebinding: the overrides in acc_settings.ini, conflicts, and which actions may be rebound.

## Declarations (in source order)

- L19 `void BindKey(Action a, char* out, size_t cap)`
- L23 `bool ParseBinding(const char* s, Binding& out)`
- L35 `void Persist(Action a, const Binding& b)`
- L47 `void EnsureOverridesLoaded()`: Retries every tick until the settings path is known, so an early call does not lock in the defaults.
- L69 `void SetUserBinding(Action a, Binding b)`
- L78 `void ResetUserBindings()`: The defaults are written back too, so no stale override stays in the file.
- L88 `Action FindConflict(Action self, int vk, uint32_t mods)`

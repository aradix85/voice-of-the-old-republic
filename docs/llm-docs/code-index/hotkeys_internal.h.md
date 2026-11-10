# hotkeys_internal.h (38 lines)

Source: `patches/Accessibility/hotkeys_internal.h`

Internal to the hotkeys module (hotkeys*.cpp); no other file includes it. The public interface is hotkeys.h. Here: what the files share.

## Declarations (in source order)

- L16 `extern Binding g_bindings[static_cast<int>(Action::COUNT)]`: Indexed by Action.
- L17 `extern Binding g_defaults[static_cast<int>(Action::COUNT)]`
- L20 `inline bool UsesKey(const Binding& b, int vk)`: True when `b` sits on key `vk`, as its primary or its alternate key.
- L25 `inline bool ModsSatisfy(const Binding& b, uint32_t mods)`: True when `mods` holds every modifier `b` requires and none it forbids.
- L32 `void InitDefaults()`: hotkeys_defaults.cpp
- L35 `void EnsureOverridesLoaded()`: hotkeys_user.cpp

Also 1 constants or variables at namespace scope.

# hotkeys_internal.h (40 lines)

Source: `patches/Accessibility/hotkeys_internal.h`

Internal to the hotkeys module (hotkeys*.cpp); no other file includes it. The public interface is hotkeys.h. Here: what the files share.

## Declarations (in source order)

- L23 `extern Binding g_bindings[static_cast<int>(Action::COUNT)]`: ----- Default bindings ------------------------------------------------------ One row per `Action`.
- L29 `extern Binding g_defaults[static_cast<int>(Action::COUNT)]`: Factory defaults, frozen at InitDefaults time before any user override is applied.
- L34 `void InitDefaults()`: hotkeys_defaults.cpp
- L37 `void EnsureOverridesLoaded()`: hotkeys_user.cpp

# interact_internal.h (26 lines)

Source: `patches/Accessibility/interact_internal.h`

Internal to the interact files (interact_dispatch.cpp, interact_bare_keys.cpp) and input_poll_router.cpp, which calls them per tick. The public surface for other subsystems is interact_dispatch.h.

## Declarations (in source order)

- L13 `bool ShouldSwitchFromInGameMenu()`: True when a bare interact key should first switch out of the in-game menu strip rather than dispatching.
- L17 `void OnInteract(bool forceRadial)`: Enter / Shift+Enter interact on the current narrated target.
- L22 `void AnnounceBarePersonalKey(int slot)`: Bare 1-7: announce an empty target row / personal action-bar column.
- L23 `void AnnounceBareTargetKey(int row)`

# interact_bare_keys.cpp (98 lines)

Source: `patches/Accessibility/interact_bare_keys.cpp`

The bare action-bar keys 1-7: the announcement when the engine refused (an empty column or row); a real queue add is announced by the AddAction detour.

## Declarations (in source order)

- L21 `void SpeakColumnEmpty(int index, char* msg, size_t cap)`: The bare action-bar keys: the empty-column phrase, 1-based for the user.
- L33 `void AnnounceBarePersonalKey(int slot)`: Bare keys 4-7 fire personal action-bar columns 0-3 in the engine itself (no hook on DoPersonalAction).
- L71 `void AnnounceBareTargetKey(int row)`: Bare keys 1-3: the target-action rows 0-2, the same way.

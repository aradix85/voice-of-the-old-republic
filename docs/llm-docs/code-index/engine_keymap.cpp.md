# engine_keymap.cpp (537 lines)

Source: `patches/Accessibility/engine_keymap.cpp`

queries at the bottom of this file

## Declarations (in source order)

- L111 `bool IsDownVk(int vk)`
- L115 `void AddAxisVk(int axis, int vk)`
- L131 `int InputIndexToScancode(int ii)`: Engine InputIndices value -> DIK (PS/2 set-1) scancode, DIRECTLY.
- L196 `void AddPair(int code, int vk)`
- L206 `void AddGameVk(int vk)`
- L224 `bool ResolveIniPath(char* out, size_t cap)`: Resolve the game's config ini from acclog::PatchDir() (= \patches) by stripping the last path component — the same derivation mod_settings_store uses.
- L238 `int ScancodeToVk(int scancode)`
- L249 `int InputIndexToVk(int ii)`
- L311 `void ReloadGameConfig()`
- L424 `bool IsKeyUsedByGame(int vk)`
- L434 `int GameActionVk(int actionId)`
- L442 `int GameActionScancode(int actionId)`
- L450 `void Rebuild()`
- L466 `int VksForCode(int code, int* out, int cap)`
- L480 `int CodeForVk(int vk)`
- L488 `int MoveAxisPrimaryVk(MoveAxis axis)`
- L495 `int TurnScancode(bool left)`
- L502 `bool ForwardBackwardKeyHeld()`
- L514 `int FirstMovementKeyHeld()`
- L532 `bool AnyMovementKeyHeld()`

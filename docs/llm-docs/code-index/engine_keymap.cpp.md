# engine_keymap.cpp (550 lines)

Source: `patches/Accessibility/engine_keymap.cpp`

queries at the bottom of this file

## Declarations (in source order)

- L112 `bool IsDownVk(int vk)`
- L116 `void AddAxisVk(int axis, int vk)`
- L132 `int InputIndexToScancode(int ii)`: Engine InputIndices value -> DIK (PS/2 set-1) scancode, DIRECTLY.
- L198 `void AddPair(int code, int vk)`
- L208 `void AddGameVk(int vk)`
- L226 `bool ResolveIniPath(char* out, size_t cap)`: Resolve the game's config ini from acclog::PatchDir() (= \patches) by stripping the last path component — the same derivation mod_settings_store uses.
- L240 `int ScancodeToVk(int scancode)`
- L251 `int InputIndexToVk(int ii)`
- L313 `void ReloadGameConfig()`
- L426 `bool IsKeyUsedByGame(int vk)`
- L436 `int GameActionVk(int actionId)`
- L444 `int GameActionScancode(int actionId)`
- L452 `void Rebuild()`
- L468 `int VksForCode(int code, int* out, int cap)`
- L482 `int CodeForVk(int vk)`
- L490 `int MoveAxisVks(MoveAxis axis, int* out, int cap)`
- L501 `int MoveAxisPrimaryVk(MoveAxis axis)`
- L508 `int TurnScancode(bool left)`
- L515 `bool ForwardBackwardKeyHeld()`
- L527 `int FirstMovementKeyHeld()`
- L545 `bool AnyMovementKeyHeld()`

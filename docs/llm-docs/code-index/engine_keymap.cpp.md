# engine_keymap.cpp (588 lines)

Source: `patches/Accessibility/engine_keymap.cpp`

queries at the bottom of this file

## Declarations (in source order)

- L127 `bool IsDownVk(int vk)`
- L131 `void AddAxisVk(int axis, int vk)`
- L147 `int InputIndexToScancode(int ii)`: Engine InputIndices value -> DIK (PS/2 set-1) scancode, DIRECTLY.
- L213 `void AddPair(int code, int vk)`
- L223 `void AddGameVk(int vk)`
- L241 `bool ResolveIniPath(char* out, size_t cap)`: Resolve the game's config ini from acclog::PatchDir() (= \patches) by stripping the last path component — the same derivation mod_settings_store uses.
- L255 `int ScancodeToVk(int scancode)`
- L266 `int InputIndexToVk(int ii)`
- L328 `void ReloadGameConfig()`
- L449 `bool IsKeyUsedByGame(int vk)`
- L459 `int GameActionVk(int actionId)`
- L467 `int GameActionScancode(int actionId)`
- L475 `int MoveAxisScancode(MoveAxis axis)`
- L483 `void Rebuild()`
- L499 `int VksForCode(int code, int* out, int cap)`
- L513 `int CodeForVk(int vk)`
- L521 `int MoveAxisVks(MoveAxis axis, int* out, int cap)`
- L532 `int MoveAxisPrimaryVk(MoveAxis axis)`
- L539 `int TurnScancode(bool left)`
- L546 `int MinigameSteerScancode(bool left)`
- L553 `bool ForwardBackwardKeyHeld()`
- L565 `int FirstMovementKeyHeld()`
- L583 `bool AnyMovementKeyHeld()`

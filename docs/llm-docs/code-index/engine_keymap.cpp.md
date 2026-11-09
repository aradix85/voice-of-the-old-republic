# engine_keymap.cpp (597 lines)

Source: `patches/Accessibility/engine_keymap.cpp`

queries at the bottom of this file

## Declarations (in source order)

- L128 `bool IsDownVk(int vk)`
- L132 `void AddAxisVk(int axis, int vk)`
- L148 `int InputIndexToScancode(int ii)`: Engine InputIndices value -> DIK (PS/2 set-1) scancode, DIRECTLY.
- L214 `void AddPair(int code, int vk)`
- L224 `void AddGameVk(int vk)`
- L242 `bool ResolveIniPath(char* out, size_t cap)`: Resolve the game's config ini from acclog::PatchDir() (= \patches) by stripping the last path component — the same derivation mod_settings_store uses.
- L256 `int ScancodeToVk(int scancode)`
- L267 `int InputIndexToVk(int ii)`
- L329 `void ReloadGameConfig()`
- L450 `bool IsKeyUsedByGame(int vk)`
- L460 `int GameActionVk(int actionId)`
- L468 `int GameActionScancode(int actionId)`
- L476 `int MoveAxisScancode(MoveAxis axis)`
- L484 `void Rebuild()`
- L500 `int VksForCode(int code, int* out, int cap)`
- L514 `int CodeForVk(int vk)`
- L522 `int MoveAxisVks(MoveAxis axis, int* out, int cap)`
- L533 `int MoveAxisPrimaryVk(MoveAxis axis)`
- L540 `int TurnScancode(bool left)`
- L547 `int MinigameSteerScancode(bool left)`
- L554 `bool ForwardBackwardKeyHeld()`
- L566 `int FirstMovementKeyHeld()`
- L584 `bool AnyMovementKeyHeld()`
- L588 `bool ForwardBackwardCommanded()`
- L592 `bool AnyMovementCommanded()`

# engine_keymap.cpp (444 lines)

Source: `patches/Accessibility/engine_keymap.cpp`

queries at the bottom of this file

## Declarations (in source order)

- L112 `bool IsDownVk(int vk)`
- L116 `void AddAxisVk(int axis, int vk)`
- L165 `void AddPair(int code, int vk)`
- L175 `void AddGameVk(int vk)`
- L193 `bool ResolveIniPath(char* out, size_t cap)`: Resolve the game's config ini from acclog::PatchDir() (= \patches) by stripping the last path component — the same derivation mod_settings_store uses.
- L207 `int ScancodeToVk(int scancode)`
- L218 `void ReloadGameConfig()`
- L331 `bool IsKeyUsedByGame(int vk)`
- L341 `int GameActionVk(int actionId)`
- L349 `int GameActionScancode(int actionId)`
- L357 `void Rebuild()`
- L373 `int VksForCode(int code, int* out, int cap)`
- L387 `int CodeForVk(int vk)`
- L395 `int MoveAxisPrimaryVk(MoveAxis axis)`
- L402 `int TurnScancode(bool left)`
- L409 `bool ForwardBackwardKeyHeld()`
- L421 `int FirstMovementKeyHeld()`
- L439 `bool AnyMovementKeyHeld()`

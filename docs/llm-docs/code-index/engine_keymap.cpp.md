# engine_keymap.cpp (434 lines)

Source: `patches/Accessibility/engine_keymap.cpp`

(From engine_keymap.h.) Engine keybinding table — maps the engine's in-world / GUI-manager command codes back to the physical-key VKs that produce them, so the input hooks can tell when one of our modifier-using mod hotkeys is shadowing an engine-bound key.

Why this exists --------------- KOTOR reads DirectInput scancodes, which are modifier-blind: pressing Shift+4 still makes the engine emit the bare "4" action and fire DoPersonalAction. Every mod hotkey that reuses an engine-bound key with a modifier (Shift+1..7 open the action submenus, Shift+L opens level-up, …) therefore double-fires the engine's bare action. The input hooks consult this table on every press: if the live modifier state matches a registered mod binding on the physical key the engine code represents, they swallow the engine event.

Namespace note -------------- The codes here are the engine's internal "quick action" COMMAND codes (what CClientExoAppInternal::HandleInputEvent receives), NOT the swkotor.ini [Keymapping] ActionNNN ids — those are a separate namespace (confirmed: command 214 = Equip/U, but [Keymapping] Action214 = F4). The shadowed gameplay hotkeys (action-menu digits, quick-menu letters) are HARDCODED in the engine — they are not exposed in the in-game Key Mapping screen — so they are stable regardless of the user's game keybinds. We map command -> DIK scancode from the engine's hardcoded handler and resolve scancode -> VK via MapVirtualKeyEx against the ACTIVE keyboard layout, so the result matches what the mod's GetAsyncKeyState-based bindings see on the same physical key (QWERTY/QWERTZ/AZERTY correctness).

KOTOR 2 is the exception: there the command codes are the [Keymapping] action ids, so its quick-screen pairs are read from swkotor2.ini (see kQuickScreenCommands in engine_keymap.cpp for the measurement).

## Declarations (in source order)

- L112 `void AddAxisVk(int axis, int vk)`
- L170 `void AddPair(int code, int vk)`
- L180 `void AddGameVk(int vk)`
- L193 `bool ResolveIniPath(char* out, size_t cap)`: Resolve the game's config ini from acclog::PatchDir() (= \patches) by stripping the last path component — the same derivation mod_settings_store uses.
- L207 `int ScancodeToVk(int scancode)`
- L222 `void SeedDefaults()`: WASD in every axis bucket and primary bind, so neither is ever empty, whatever happens to the ini.
- L235 `void ApplyEntry(const KeymapEntry& e)`: Files one [Keymapping] entry into the tables.
- L270 `void ReloadGameConfig()`
- L312 `bool IsKeyUsedByGame(int vk)`
- L322 `int GameActionVk(int actionId)`
- L330 `int GameActionScancode(int actionId)`
- L338 `void Rebuild()`
- L363 `int VksForCode(int code, int* out, int cap)`
- L377 `int CodeForVk(int vk)`
- L385 `int MoveAxisPrimaryVk(MoveAxis axis)`
- L392 `int TurnScancode(bool left)`
- L399 `bool ForwardBackwardKeyHeld()`
- L411 `int FirstMovementKeyHeld()`
- L429 `bool AnyMovementKeyHeld()`

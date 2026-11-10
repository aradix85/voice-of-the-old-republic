# engine_keymap.h (159 lines)

Source: `patches/Accessibility/engine_keymap.h`

Engine keybinding table — maps the engine's in-world / GUI-manager command codes back to the physical-key VKs that produce them, so the input hooks can tell when one of our modifier-using mod hotkeys is shadowing an engine-bound key.

Why this exists --------------- KOTOR reads DirectInput scancodes, which are modifier-blind: pressing Shift+4 still makes the engine emit the bare "4" action and fire DoPersonalAction. Every mod hotkey that reuses an engine-bound key with a modifier (Shift+1..7 open the action submenus, Shift+L opens level-up, …) therefore double-fires the engine's bare action. The input hooks consult this table on every press: if the live modifier state matches a registered mod binding on the physical key the engine code represents, they swallow the engine event.

Namespace note -------------- The codes here are the engine's internal "quick action" COMMAND codes (what CClientExoAppInternal::HandleInputEvent receives), NOT the swkotor.ini [Keymapping] ActionNNN ids — those are a separate namespace (confirmed: command 214 = Equip/U, but [Keymapping] Action214 = F4). The shadowed gameplay hotkeys (action-menu digits, quick-menu letters) are HARDCODED in the engine — they are not exposed in the in-game Key Mapping screen — so they are stable regardless of the user's game keybinds. We map command -> DIK scancode from the engine's hardcoded handler and resolve scancode -> VK via MapVirtualKeyEx against the ACTIVE keyboard layout, so the result matches what the mod's GetAsyncKeyState-based bindings see on the same physical key (QWERTY/QWERTZ/AZERTY correctness).

## Declarations (in source order)

- L37 `void Rebuild()`: (Re)build the table for the active keyboard layout.
- L42 `int VksForCode(int code, int* out, int cap)`: Fill `out` with up to `cap` distinct VKs that, when pressed, make the engine emit the given command `code`.
- L47 `int CodeForVk(int vk)`: Reverse lookup: the first engine command code a given VK fires, or 0 if the VK is not in the table.
- L53 `bool IsKeyUsedByGame(int vk)`: True iff `vk` is bound to ANY game action — a hardcoded quick key (CodeForVk) OR a player-configurable bind read from swkotor.ini [Keymapping].
- L58 `void ReloadGameConfig()`: (Re)read the configurable [Keymapping] binds from swkotor.ini.
- L64 `int GameActionVk(int actionId)`: The VK currently bound to a slotless [Keymapping] game action ("Action =" lines, e.g.
- L72 `int GameActionScancode(int actionId)`: The same bind as a DIK scancode, for a caller that has to SYNTHESISE the key rather than test it — the KOTOR 1 pad's quick menu drives Solo Mode, Stealth, Quick Save and the rest this way (the engine reads DirectInput...
- L76 `int ScancodeToVk(int scancode)`: Resolve a DirectInput scancode to a Win32 VK against the active layout.
- L82 `int InputIndexToVk(int inputIndex)`: Resolve an engine InputIndices value (KEYBOARD_MOUSE_* — what the keymap row's key_code and swkotor.ini [Keymapping] actually store) to a Win32 VK.
- L90 `enum class MoveAxis`: ---- Movement / turn axis queries ------------------------------------------ The player's bound movement + turn keys, so the mod can react to "the player is moving / turning" and steer the map cursor by the same keys ...
- L95 `int MoveAxisVks(MoveAxis axis, int* out, int cap)`: Fill `out` with up to `cap` distinct VKs bound to the given movement/turn direction (default key + any [Keymapping] binds on that direction's slots).
- L116 `int MoveAxisPrimaryVk(MoveAxis axis)`: The single VK a consumer should treat as "the" key for this direction, when reacting to every bound key would be wrong.
- L123 `int MoveAxisScancode(MoveAxis axis)`: The same primary bind as a DIK scancode.
- L131 `int TurnScancode(bool left)`: DIK scancode to synthesise (via SendInput KEYEVENTF_SCANCODE) to drive the engine's camera-turn axis in the given direction — resolved from the player's configured primary turn bind (Action283/284), so it follows a re...
- L138 `int MinigameSteerScancode(bool left)`: DIK scancode of the minigame steer bind (Action283 MGActionLeft/Right), the input the swoop race polls to steer.
- L143 `bool AnyMovementKeyHeld()`: True iff any bound movement/turn key is currently held down.
- L149 `int FirstMovementKeyHeld()`: Same scan, but returns the VK that answered (0 when none).
- L156 `bool ForwardBackwardKeyHeld()`: True iff a bound forward/backward key is currently held (the walk axes, including the arrow alternates) — deliberately EXCLUDING the turn/strafe buckets.

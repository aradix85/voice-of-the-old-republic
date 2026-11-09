# pad_quickmenu.h (77 lines)

Source: `patches/Accessibility/pad_quickmenu.h`

KOTOR 1: the Y button's quick menu.

KOTOR 2 ships one — `CSWGamepadMenuIos`, an eight-entry overlay the pad opens with Y — and it turned out to be an ordinary panel the mod's navigation chain can walk, so on that game there is nothing to build. KOTOR 1 has no such panel and no pad at all, so the mod supplies the surface itself.

Same shape, one entry shorter: KOTOR 1 has no Switch Weapons. What is left is the list of things a player wants without leaving the world — the menu screens, the party leader, solo mode, stealth, a quick save — plus Help, which on KOTOR 2 the mod also claims because the engine's own Help entry opens a picture of a controller.

Every game entry fires by synthesising the player's OWN bound key for that [Keymapping] action (engine_keymap::GameActionScancode + key_inject), so a rebind is honoured and nothing here needs to know how the engine performs the action. Help is the exception: it opens the mod's key list directly.

Deliberately does NOT pause the world. It is an overlay over a running game, exactly as it is on KOTOR 2, and the entries are one-shot actions rather than something to deliberate over. The left stick IS stood down while it is open (see pad_movement), so the player cannot walk off mid-choice.

Modelled on the F1 help list (help.cpp): an in-DLL overlay with no engine panel of its own, navigated by whatever routes codes into it. Up/Down clamp at the ends rather than wrapping — the repeated line is the boundary cue, the same rule every submenu in this mod follows.

## Declarations (in source order)

- L34 `bool IsOpen()`: True while the list is up and owns the pad's navigation.
- L36 `void Open()`
- L37 `void Close()`
- L38 `void Toggle()`
- L43 `bool HandleNavCode(int code)`: Drive the list from the pad seam.
- L47 `void Tick()`: Self-disarm: closes if the world drops out from under it (area load, teardown).
- L74 `bool FilterEngineMenuInput(void* panel, int code, int value, bool& consume)`: Manager-hook filter.

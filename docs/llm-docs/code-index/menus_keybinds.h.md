# menus_keybinds.h (58 lines)

Source: `patches/Accessibility/menus_keybinds.h`

Mod keybind configurator — the "Tastenbelegung" submenu reached from Mod settings. Lets the user freely rebind every user-rebindable hotkeys::Action.

Layer: menus/ (UI state + speech). Like the Audio glossary, this is an entirely virtual submenu (no engine panel): menus_modsettings owns an "open" flag and routes navigation keys here while it is set.

Two-level navigation, mirroring the engine Key Mapping screen we made accessible (menus_keymap.cpp):

* Category level — the five categories plus a "restore defaults" entry. Up/Down move, Enter drills into a category (or runs the reset), Esc closes back to the Mod-settings root. * Action level — each row reads "{name}: {bind}". Enter arms capture; Esc returns to the category level.

Capture is our own (the engine's SetCaptureEvent only serves engine panels): while armed, Tick() polls Win32 for the next physical key plus the modifiers held and writes the new binding. A clash with another mod binding, or with a hardcoded game key, is announced and capture stays armed for another try. Esc cancels capture.

## Declarations (in source order)

- L33 `const char* DisplayName(acc::hotkeys::Action action)`: Localised display name for a user-rebindable action, or "" if the action is not in the configurator catalogue (e.g.
- L36 `bool IsOpen()`: True while the configurator owns input (any level, including capture).
- L40 `void Open()`: Open at the category level and announce the title + first category.
- L46 `bool HandleInput(int keyCode)`: Press-edge input handler.
- L50 `void Tick()`: Per-frame poll.
- L55 `void Reset()`: Force-close without speech.

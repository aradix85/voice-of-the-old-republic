# menus_modsettings.h (72 lines)

Source: `patches/Accessibility/menus_modsettings.h`

The virtual "Mod settings" submenu, reached through an entry the chain adds to the in-game and the title-screen Options panels.

The entry is not an engine control: its `control` is a sentinel pointer (IsRootAnchor), which menus_extract speaks as "Mod settings" and the Enter dispatcher opens with OpenSubMenu. The submenu has no engine panel either; while IsOpen(), the input hook routes keys through HandleInput first. The audio glossary and the keybind configurator are nested views; Esc inside one returns to the list.

## Declarations (in source order)

- L20 `enum class Option`: A row's stable identity (persistence keys, GetToggle callers), not its place on screen.
- L38 `bool IsRootAnchor(void* control)`: True iff `control` is the root entry's sentinel.
- L43 `void ForEachRootAnchor(void* panel, bool (*callback)(void* sentinel, int sortCx, int sortCy, void* userData), void* userData)`: Calls `callback` once with the root sentinel and its sort position on InGameOptions and MainMenuOptions (the last stop of the chain); nothing for other panels.
- L49 `bool ExtractRootLabel(char* outBuf, size_t bufSize)`: The root entry's speech ("Mod settings"); false only for a degenerate buffer.
- L52 `void OpenSubMenu(void* parentPanel)`: Opens the submenu over `parentPanel` and speaks the title and first row.
- L54 `bool IsOpen()`
- L58 `void Close()`: Closes the submenu and rebinds the chain on the parent panel; no-op when closed.
- L62 `bool HandleInput(int keyCode)`: Press edges only.
- L65 `bool GetToggle(Option option)`: The state of a toggle option; false for out-of-range and non-toggle rows.
- L69 `void Tick()`: Per-frame: the keybind configurator's capture poll and the glossary's delayed audition.

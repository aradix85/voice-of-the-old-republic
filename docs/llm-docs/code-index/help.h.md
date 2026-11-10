# help.h (55 lines)

Source: `patches/Accessibility/help.h`

Help system — two surfaces over one tagged keybind catalog.

F1 (HelpMenuOpen)   — opens a navigable, screen-reader-friendly list of every important binding, grouped under section headers. Up/Down read the list, Home/End jump to the ends, Enter re-reads the current line, Esc or F1 close. Works in EVERY context (in-world, any menu, dialog, map) because it is a synthetic in-DLL overlay driven by a global Win32 poll — no engine panel of its own. Mirrors examine_view's model.

Ctrl+F1 (HelpContext) — speaks the subset of the catalog tagged for the screen the user is currently on (in-world / menu / map / action menu / dialog / container / store). Generalises the per-screen "what's important here" announcement the Pazaak board already does on open.

The catalog (kEntries in help.cpp) is the single source of truth. Each entry carries a `grp` (which F1 section it lives under), a curated `ctx` bitmask (which screens speak it on Ctrl+F1 — kept short so a context summary stays digestible), and a `k2Only` flag. F1 is exhaustive; Ctrl+F1 is the essentials, ending with a "press F1 for the full list" hint.

The strings live in strings.h (HelpGroup* / HelpKey* / HelpContext*).

## Declarations (in source order)

- L34 `bool IsMenuOpen()`: True while the F1 list overlay is interactive.
- L39 `void OpenMenu()`: Open / close / toggle the F1 list.
- L40 `void CloseMenu()`
- L47 `void PollWin32()`: Global per-tick poll.
- L52 `void Tick()`: Self-disarm hook.

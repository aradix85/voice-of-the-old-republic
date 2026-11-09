# help.h (80 lines)

Source: `patches/Accessibility/help.h`

Help system — two surfaces over one tagged keybind catalog.

F1 (HelpMenuOpen)   — opens a navigable, screen-reader-friendly list of every important binding, grouped under section headers. Up/Down read the list, Home/End jump to the ends, Enter re-reads the current line, Esc or F1 close. Works in EVERY context (in-world, any menu, dialog, map) because it is a synthetic in-DLL overlay driven by a global Win32 poll — no engine panel of its own. Mirrors examine_view's model.

Ctrl+F1 (HelpContext) — speaks the subset of the catalog tagged for the screen the user is currently on (in-world / menu / map / action menu / dialog / container / store). Generalises the per-screen "what's important here" announcement the Pazaak board already does on open.

The catalog (kEntries in help.cpp) is the single source of truth. Each entry carries a `grp` (which F1 section it lives under), a curated `ctx` bitmask (which screens speak it on Ctrl+F1 — kept short so a context summary stays digestible), and a `dev` tag. F1 is exhaustive within its device; Ctrl+F1 is the essentials, ending with a "press F1 for the full list" hint.

Both surfaces answer for ONE device at a time. With a gamepad present the list is the controller vocabulary, and the keyboard rows it duplicates are not read out; without one it is the keyboard's, and the controller rows are absent. That is the console convention, and it is the same predicate every tutorial hint switches on (binding_hints::ControllerActive), so the F1 list and a tutorial popup two seconds later can never name different controls.

It is also a promise the catalog has to keep: a keyboard row is HIDDEN once a pad is connected, so every one of them needs a controller row that answers the same question, or a pad player never learns the feature exists. The one exception is `Dev::Both` — a row that names no control at all and is therefore true on either device.

Localised strings live in strings.h (HelpGroup* / HelpKey* / HelpContext*).

## Declarations (in source order)

- L47 `bool IsMenuOpen()`: True while the F1 list overlay is interactive.
- L52 `void OpenMenu()`: Open / close / toggle the F1 list.
- L53 `void CloseMenu()`
- L60 `void PollWin32()`: Global per-tick poll.
- L65 `void ToggleMenu()`: Toggle the list / speak the current screen's keys from a source that is not the keyboard poll — the KOTOR 2 gamepad reaches both surfaces through the trigger layer (see pad_input.h), and a pad user has no other way in.
- L66 `void SpeakContextHelp()`
- L72 `bool HandleNavCode(int code)`: Drive the open list from a non-keyboard source.
- L77 `void Tick()`: Self-disarm hook.

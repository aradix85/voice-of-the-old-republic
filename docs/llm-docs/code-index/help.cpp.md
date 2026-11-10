# help.cpp (521 lines)

Source: `patches/Accessibility/help.cpp`

Help system — see help.h for the design summary.

One catalog (kEntries) feeds both surfaces. `grp` decides the F1 section a line is listed under (F1 lists every entry); `ctx` is a curated bitmask of the screens that speak the line on Ctrl+F1 (kept short on purpose). Adding a keybind = add a strings::Id (strings.h) + one row here.

## Declarations (in source order)

- L50 `uint32_t ContextBit(Context c)`
- L64 `S ContextNameId(Context c)`
- L80 `Context DetectContext()`: Detect the screen the user is on.
- L264 `void BuildComposedText(S formatId, char* out, size_t cap)`: Resolve a composed entry's text.
- L292 `bool EntryListed(const Entry& e)`: Build the flat row list: each non-empty group emits a header row followed by its entries, in catalog order.
- L296 `void BuildRows()`
- L338 `void SpeakRow(int idx, bool interrupt)`
- L357 `void StepFocus(int delta)`: Focus movement.
- L363 `void FocusEdge(bool last)`
- L369 `void SpeakContext()`: Ctrl+F1 — speak the keys tagged for the current screen, joined.
- L410 `bool IsMenuOpen()`
- L412 `void OpenMenu()`
- L433 `void CloseMenu()`
- L446 `void ToggleMenu()`
- L451 `void PollWin32()`
- L500 `void Tick()`

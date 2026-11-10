# help.cpp (520 lines)

Source: `patches/Accessibility/help.cpp`

Help system — see help.h for the design summary.

One catalog (kEntries) feeds both surfaces. `grp` decides the F1 section a line is listed under (F1 lists every entry); `ctx` is a curated bitmask of the screens that speak the line on Ctrl+F1 (kept short on purpose). Adding a keybind = add a strings::Id (strings.h) + one row here.

## Declarations (in source order)

- L50 `uint32_t ContextBit(Context c)`
- L64 `S ContextNameId(Context c)`
- L80 `Context DetectContext()`: Detect the screen the user is on.
- L263 `void BuildComposedText(S formatId, char* out, size_t cap)`: Resolve a composed entry's text.
- L291 `bool EntryListed(const Entry& e)`: Build the flat row list: each non-empty group emits a header row followed by its entries, in catalog order.
- L295 `void BuildRows()`
- L337 `void SpeakRow(int idx, bool interrupt)`
- L356 `void StepFocus(int delta)`: Focus movement.
- L362 `void FocusEdge(bool last)`
- L368 `void SpeakContext()`: Ctrl+F1 — speak the keys tagged for the current screen, joined.
- L409 `bool IsMenuOpen()`
- L411 `void OpenMenu()`
- L432 `void CloseMenu()`
- L445 `void ToggleMenu()`
- L450 `void PollWin32()`
- L499 `void Tick()`

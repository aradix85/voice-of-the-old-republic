# help.cpp (685 lines)

Source: `patches/Accessibility/help.cpp`

Help system — see help.h for the design summary.

One catalog (kEntries) feeds both surfaces. `grp` decides the F1 section a line is listed under (F1 lists every entry); `ctx` is a curated bitmask of the screens that speak the line on Ctrl+F1 (kept short on purpose). Adding a keybind = add a strings::Id (strings.h) + one row here.

## Declarations (in source order)

- L52 `uint32_t ContextBit(Context c)`
- L66 `S ContextNameId(Context c)`
- L82 `Context DetectContext()`: Detect the screen the user is on.
- L160 `bool ControllerWording()`: One test for every row, so the F1 list and the Ctrl+F1 summary can never disagree about which device the player is on — and, because it is the same predicate the tutorial hints ask, so the help list and a tutorial pop...
- L393 `void BuildComposedText(S formatId, char* out, size_t cap)`: Resolve a composed entry's text.
- L421 `bool EntryListed(const Entry& e)`: Build the flat row list: each non-empty group emits a header row followed by its entries, in catalog order.
- L431 `void BuildRows()`
- L474 `void SpeakRow(int idx, bool interrupt)`
- L494 `void StepFocus(int delta)`: Focus movement, shared by the keyboard poll and the pad nav route so the two can never drift.
- L500 `void FocusEdge(bool last)`
- L506 `void SpeakContext()`: Ctrl+F1 — speak the keys tagged for the current screen, joined.
- L547 `bool IsMenuOpen()`
- L549 `void OpenMenu()`
- L570 `void CloseMenu()`
- L583 `void ToggleMenu()`
- L588 `void SpeakContextHelp()`
- L590 `bool HandleNavCode(int code)`
- L615 `void PollWin32()`
- L664 `void Tick()`

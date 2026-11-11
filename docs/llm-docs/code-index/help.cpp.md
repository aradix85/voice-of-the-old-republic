# help.cpp (379 lines)

Source: `patches/Accessibility/help.cpp`

The help system; help.h has the design. One catalog (kEntries) feeds both surfaces: `grp` is the F1 section a line is listed under (F1 lists every entry), `ctx` the screens that speak the line on Ctrl+F1 (kept short on purpose). A new keybind is a strings::Id (strings.h) and one row here.

## Declarations (in source order)

- L30 `const char* Str(S id)`
- L44 `S ContextName(Context c)`
- L60 `Context DetectContext()`: The screen the user is on: the most specific surfaces first, then "some menu is up", the world last.
- L189 `bool EntryListed(const Entry& e)`: A line that KOTOR 2 alone has is not listed on KOTOR 1.
- L196 `void EntryText(S label, char* out, size_t cap)`: An entry's line.
- L233 `void BuildRows()`
- L251 `void SpeakRow(int idx, bool interrupt)`
- L267 `void FocusRow(int idx)`: Clamped, no wrap: the repeated line is the boundary cue.
- L274 `void SpeakContext()`: Ctrl+F1: the lines tagged for the current screen, joined.
- L303 `void Open()`
- L323 `void Close(bool announce)`
- L336 `bool Took(acc::hotkeys::Action a)`: This tick's press of `a`, claimed so that no later poller (cycle, interact, examine, the Home/End synthesiser) acts on it too.
- L344 `bool IsMenuOpen()`
- L346 `void PollWin32()`
- L368 `void Tick()`

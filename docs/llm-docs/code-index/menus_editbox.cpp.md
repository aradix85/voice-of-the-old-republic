# menus_editbox.cpp (290 lines)

Source: `patches/Accessibility/menus_editbox.cpp`

The editbox: arms when chain focus lands on one, speaks each typed or deleted character from a per-tick text diff, and handles the re-read, submit and cancel keys (menus_editbox.h has the model). Vanilla has two editbox panels (chargen Name and the save-name popup), so one armed editbox at a time.

## Declarations (in source order)

- L52 `bool Matches(const EditboxPanelSpec& spec, void* panel)`
- L57 `void* At(void* panel, size_t offset)`
- L61 `const EditboxPanelSpec* SpecFor(void* panel)`
- L79 `void Snapshot(ArmedState& s)`
- L83 `void SpeakFullText(const char* text, uint32_t len)`
- L89 `void SpeakSingleChar(char c)`: One character as a one-char string, read per NVDA's character verbosity.
- L97 `void PollAndAnnounceDiff(ArmedState& s)`: CSWGuiEditbox has no caret (a struct-wide diff against 21 Left/Right presses showed nothing), so one character more is taken as appended and one fewer as Backspace from the end; Up/Down re-read the whole value.
- L126 `void PollModalKeys(ArmedState& s)`: The engine's text input eats the keys at the editbox before the manager hook sees them (no Menus.Input line during typing), so the modal keys are polled.
- L167 `PanelMatch FindMatchingPanel()`
- L179 `void DisarmIfArmed(const char* reason)`
- L189 `void Arm(const PanelMatch& m, void* editbox)`: Focus has landed on the editbox: arm, swallow the edges of keys still held (the Enter that opened the editbox fired a submit on the first poll), and speak "Editbox.
- L212 `bool TryHandleInput(int /*n*/, void* /*thisPtr*/, void* activePanel, int param_1, int param_2, int& outRv)`
- L252 `void TickEditboxMonitors()`
- L279 `const char* GetTitleOverride(void* panel)`

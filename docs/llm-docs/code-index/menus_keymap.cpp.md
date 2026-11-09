# menus_keymap.cpp (482 lines)

Source: `patches/Accessibility/menus_keymap.cpp`

Tastaturbelegung screen — see menus_keymap.h for the design rationale.

## Declarations (in source order)

- L98 `void* ResolveTabEntry(void* panel, int idx)`: The control a tab entry reads/activates.
- L137 `void* RowAt(void* panel, short index)`: Row control at `index`, or nullptr if the list is shorter than that.
- L159 `void SetSelectionIndex(void* lb, short v)`: Force the listbox's selection_index back to OUR cursor before a step.
- L167 `int CaptureActive(void* panel)`
- L177 `void SpeakControl(void* panel, void* ctrl)`: Speak a control's text (entry name or "{action}: {key}" row readout).
- L186 `void AnnounceTabEntry(void* panel, int idx)`
- L195 `bool StepRow(void* panel, ListBoxNavOp op)`: Step the binding list from OUR remembered cursor and announce the row we land on.
- L221 `void SwitchCategory(void* panel, int filterIndex)`: Switch the displayed category by calling OnFilter* directly (synchronous, so the listbox is repopulated before we browse it).
- L249 `void ArmCapture(void* panel, void* row)`
- L280 `void WarnIfModBindClash(void* row)`: After the engine applies a freshly captured key to a game action, warn if that same physical key also drives a mod hotkey.
- L307 `bool IsKeyMapPanel(void* panel)`
- L313 `void Tick()`
- L346 `bool HandleInput(void* activePanel, int param_1, int param_2, int& outRv)`

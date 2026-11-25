# menus_keymap.cpp (439 lines)

Source: `patches/Accessibility/menus_keymap.cpp`

Keymapping screen — see menus_keymap.h for the design rationale.

## Declarations (in source order)

- L98 `void* ResolveTabEntry(void* panel, int idx)`: The control a tab entry reads/activates.
- L135 `void* KeyboardRow(void* panel)`: The binding row the keyboard is on.
- L141 `int CaptureActive(void* panel)`
- L146 `void SpeakControl(void* panel, void* ctrl)`: Speak a control's text (entry name or "{action}: {key}" row readout).
- L155 `void AnnounceTabEntry(void* panel, int idx)`
- L164 `bool StepRow(void* panel, ListBoxNavOp op)`: Step the binding list and announce the row we land on.
- L188 `void SwitchCategory(void* panel, int filterIndex)`: Switch the displayed category by calling OnFilter* directly (synchronous, so the listbox is repopulated before we browse it).
- L216 `void ArmCapture(void* panel, void* row)`
- L247 `void WarnIfModBindClash(void* row)`: After the engine applies a freshly captured key to a game action, warn if that same physical key also drives a mod hotkey.
- L269 `bool IsKeyMapPanel(void* panel)`
- L275 `void Tick()`
- L310 `bool HandleInput(void* activePanel, int param_1, int param_2, int& outRv)`

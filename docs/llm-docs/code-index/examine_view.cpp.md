# examine_view.cpp (153 lines)

Source: `patches/Accessibility/examine_view.cpp`

The examine panel: a read-only list of rows about the current target (name, faction, condition, hit points, level, distance, flags, equipment, effects, feats), walked with Up/Down while the world is paused.

## Declarations (in source order)

- L23 `void SpeakRow(int idx)`
- L32 `bool InWorld()`
- L37 `void Close(const char* reason)`
- L45 `bool Refresh(const char* lostReason)`: Rows are rebuilt on every step, so hit points and distance stay live.
- L54 `bool IsActive()`
- L56 `void ForceDisarm(const char* reason)`
- L67 `bool Open()`
- L108 `bool HandleInputEvent(int code, int value)`
- L137 `void Tick()`
- L142 `void PollWin32Hotkey()`: The ExamineOpen key toggles the panel.

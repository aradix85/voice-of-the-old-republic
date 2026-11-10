# focus_guard_bigpicture.cpp (140 lines)

Source: `patches/Accessibility/focus_guard_bigpicture.cpp`

The input-blocked warning: keys pressed while Steam Big Picture holds the foreground never reach the game, so the poll thread notices the keypress and the main tick speaks a warning.

## Declarations (in source order)

- L42 `bool ContainsNoCase(const char* hay, const char* needle)`: Case-insensitive substring test (ASCII).
- L58 `bool AnyInteractionKeyDown()`: True while any "the user is trying to interact" key is physically down: arrows / Enter / Space / Esc / Tab / Backspace / digits / letters / F-keys.
- L82 `bool ForegroundIsBigPicture()`: Is the current foreground window Steam Big Picture?
- L112 `void MaybeFlagInputBlockedWarning()`: Poll-thread step: if the user pressed an interaction key while Steam Big Picture holds the foreground, queue a spoken warning for the main tick to drain.
- L131 `void DrainInputBlockedWarning()`

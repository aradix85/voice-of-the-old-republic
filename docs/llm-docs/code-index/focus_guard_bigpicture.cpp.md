# focus_guard_bigpicture.cpp (96 lines)

Source: `patches/Accessibility/focus_guard_bigpicture.cpp`

The input-blocked warning. When the game runs windowed and Steam Big Picture holds the foreground, keystrokes go to Big Picture and never reach the engine, so menus look dead. The poll thread samples the physical keys and flags a warning that the main tick speaks. The foreground is never reclaimed here: that would trap a player who wants Big Picture.

## Declarations (in source order)

- L29 `bool ContainsNoCase(const char* hay, const char* needle)`
- L43 `bool AnyInteractionKeyDown()`: Any key that means "trying to interact" is physically down (modifiers alone do not count).
- L56 `bool ForegroundIsBigPicture()`: steamwebhelper.exe also backs the normal Steam client, so the title must name Big Picture too.
- L72 `void MaybeFlagInputBlockedWarning()`
- L88 `void DrainInputBlockedWarning()`: On the main tick, where speech is COM-safe.

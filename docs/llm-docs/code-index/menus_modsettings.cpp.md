# menus_modsettings.cpp (356 lines)

Source: `patches/Accessibility/menus_modsettings.cpp`

virtual mod-settings submenu.

See menus_modsettings.h for the design summary. This TU owns the sentinel pointer, the toggle bits, and the input router.

## Declarations (in source order)

- L31 `void EnsureTogglesLoaded()`: Pull persisted toggle values into s_toggles once, on first access.
- L42 `const char* StateText(Option opt)`
- L72 `bool ForegroundDivergedFromParent()`: True iff the engine pushed a new panel on top of whatever was foreground when the submenu opened — typically a MessageBoxModal raised by Alt+F4 / quit-confirm, but also any other engine modal the user hits while the s...
- L86 `const OptionSpec& Spec(Option opt)`: Spec for `opt`.
- L92 `Option FocusedOption()`: Option under the cursor, or Option::Count when the index has gone out of range.
- L101 `const char* PersistKey(Option opt)`: Stable acc_settings.ini keys for the toggles that persist across launches.
- L116 `void SpeakFocusedOption(bool interrupt)`: Speak the current focused option.
- L137 `bool IsRootAnchor(void* control)`
- L141 `void ForEachRootAnchor(void* panel, bool (*callback)(void* sentinel, int sortCx, int sortCy, void* userData), void* userData)`
- L166 `bool ExtractRootLabel(char* outBuf, size_t bufSize)`
- L180 `bool IsOpen()`
- L184 `void OpenSubMenu(void* parentPanel)`
- L208 `void Close()`
- L240 `void AutoCloseSilent()`: Silent auto-close used when the engine pushes a new panel (MessageBoxModal etc.) in front of our parent.
- L255 `bool HandleInput(int keyCode)`
- L309 `void Tick()`
- L344 `bool GetToggle(Option option)`

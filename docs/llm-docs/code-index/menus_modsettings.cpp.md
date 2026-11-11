# menus_modsettings.cpp (255 lines)

Source: `patches/Accessibility/menus_modsettings.cpp`

The mod-settings submenu: the toggle state, open and close, the virtual root entry on the Options panels, and the input router.

## Declarations (in source order)

- L54 `void EnsureTogglesLoaded()`
- L65 `const char* StateText(Option opt)`
- L74 `void* ForegroundPanel()`
- L79 `bool ForegroundDivergedFromParent()`
- L87 `void ResetState()`: Close and AutoCloseSilent differ only in what they say and the rebind.
- L101 `void AutoCloseSilent()`: A panel pushed in front of the parent announces itself and gets the chain, so this close says nothing and does not rebind.
- L109 `const OptionSpec& Spec(Option opt)`
- L113 `Option FocusedOption()`
- L119 `void SpeakFocusedOption(bool interrupt)`: interrupt=false queues the row behind a title that was just spoken.
- L138 `bool StepListCursor(int keyCode, int count, int* cursor)`
- L148 `bool IsRootAnchor(void* control)`
- L152 `void ForEachRootAnchor(void* panel, bool (*callback)(void* sentinel, int sortCx, int sortCy, void* userData), void* userData)`
- L167 `bool ExtractRootLabel(char* outBuf, size_t bufSize)`
- L174 `bool IsOpen()`
- L178 `void OpenSubMenu(void* parentPanel)`
- L190 `void Close()`
- L202 `bool HandleInput(int keyCode)`
- L241 `void Tick()`
- L246 `bool GetToggle(Option option)`

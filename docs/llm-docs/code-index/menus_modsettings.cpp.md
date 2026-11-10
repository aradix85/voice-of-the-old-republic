# menus_modsettings.cpp (254 lines)

Source: `patches/Accessibility/menus_modsettings.cpp`

The mod-settings submenu: the toggle state, open and close, the virtual root entry on the Options panels, and the input router.

## Declarations (in source order)

- L53 `void EnsureTogglesLoaded()`
- L64 `const char* StateText(Option opt)`
- L73 `void* ForegroundPanel()`
- L78 `bool ForegroundDivergedFromParent()`
- L86 `void ResetState()`: Close and AutoCloseSilent differ only in what they say and the rebind.
- L100 `void AutoCloseSilent()`: A panel pushed in front of the parent announces itself and gets the chain, so this close says nothing and does not rebind.
- L108 `const OptionSpec& Spec(Option opt)`
- L112 `Option FocusedOption()`
- L118 `void SpeakFocusedOption(bool interrupt)`: interrupt=false queues the row behind a title that was just spoken.
- L137 `bool StepListCursor(int keyCode, int count, int* cursor)`
- L147 `bool IsRootAnchor(void* control)`
- L151 `void ForEachRootAnchor(void* panel, bool (*callback)(void* sentinel, int sortCx, int sortCy, void* userData), void* userData)`
- L166 `bool ExtractRootLabel(char* outBuf, size_t bufSize)`
- L173 `bool IsOpen()`
- L177 `void OpenSubMenu(void* parentPanel)`
- L189 `void Close()`
- L201 `bool HandleInput(int keyCode)`
- L240 `void Tick()`
- L245 `bool GetToggle(Option option)`

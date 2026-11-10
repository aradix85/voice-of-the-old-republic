# menus_modsettings_glossary.cpp (125 lines)

Source: `patches/Accessibility/menus_modsettings_glossary.cpp`

The Audio glossary submenu: one row per swoop-race cue, auditioned a moment after Enter.

## Declarations (in source order)

- L47 `void SpeakFocusedGlossaryEntry()`
- L55 `void CancelPendingGlossaryCue()`: So a queued cue does not play after the user has moved on.
- L63 `void CloseGlossarySubMenu()`
- L73 `void OpenGlossarySubMenu()`
- L83 `void ResetGlossary()`
- L89 `bool HandleInputGlossary(int keyCode)`
- L111 `void TickGlossary()`

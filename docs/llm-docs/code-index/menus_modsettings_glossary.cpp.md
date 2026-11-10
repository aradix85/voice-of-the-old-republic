# menus_modsettings_glossary.cpp (135 lines)

Source: `patches/Accessibility/menus_modsettings_glossary.cpp`

The Audio glossary submenu: one row per swoop-race cue, auditioned a moment after Enter.

## Declarations (in source order)

- L34 `void SpeakFocusedGlossaryEntry()`: Speak the current focused glossary entry — just the localised name.
- L46 `void CloseGlossarySubMenu()`: Close the glossary submenu, returning the user to the root.
- L65 `void CancelPendingGlossaryCue()`: Cancel any pending delayed glossary playback.
- L76 `void OpenGlossarySubMenu()`: Open the nested Audio glossary submenu and announce its title + first row.
- L91 `bool HandleInputGlossary(int keyCode)`

# menus_modsettings.cpp (890 lines)

Source: `patches/Accessibility/menus_modsettings.cpp`

virtual mod-settings submenu.

See menus_modsettings.h for the design summary. This TU owns the sentinel pointer, the toggle bits, and the input router.

## Declarations (in source order)

- L162 `const char* ReadmeLangCode(acc::strings::Lang l)`: ISO code used in the published readme filenames (docs/README.
- L178 `const char* BuildReadmeUrl(char* buf, size_t bufSize)`: Build the readme URL for the language the mod is currently speaking in.
- L229 `const OptionSpec& Spec(Option opt)`: Spec for `opt`.
- L261 `Option FocusedOption()`: Option under the cursor, or Option::Count when the index has gone out of range.
- L270 `const char* PersistKey(Option opt)`: Stable acc_settings.ini keys for the toggles that persist across launches.
- L285 `void EnsureTogglesLoaded()`: Pull persisted toggle values into s_toggles once, on first access.
- L334 `const char* StateText(Option opt)`
- L356 `void SpeakFocusedOption(bool interrupt = true)`: Speak the current focused option.
- L380 `void SpeakFocusedGlossaryEntry()`: Speak the current focused glossary entry — just the localised name.
- L392 `void CancelPendingGlossaryCue()`: Cancel any pending delayed glossary playback.
- L412 `bool ForegroundDivergedFromParent()`: True iff the engine pushed a new panel on top of whatever was foreground when the submenu opened — typically a MessageBoxModal raised by Alt+F4 / quit-confirm, but also any other engine modal the user hits while the s...
- L425 `void* GetRootAnchor()`
- L429 `bool IsRootAnchor(void* control)`
- L433 `void ForEachRootAnchor(void* panel, bool (*callback)(void* sentinel, int sortCx, int sortCy, void* userData), void* userData)`
- L458 `bool ExtractRootLabel(char* outBuf, size_t bufSize)`
- L472 `bool IsOpen()`
- L476 `void OpenSubMenu(void* parentPanel)`
- L500 `void Close()`
- L532 `void AutoCloseSilent()`: Silent auto-close used when the engine pushes a new panel (MessageBoxModal etc.) in front of our parent.
- L552 `void OpenGlossarySubMenu()`: Open the nested Audio glossary submenu and announce its title + first row.
- L570 `void CloseGlossarySubMenu()`: Close the glossary submenu, returning the user to the root.
- L595 `bool OpenExternalUrl(const char* url)`: Hand `url` to the default browser.
- L610 `void ActivateLinkRow(Option opt)`: Enter on a Link row: open the URL, then speak the outcome.
- L628 `void AdjustSlider(int delta)`: Adjust the cue-volume slider by delta (clamped to 0..100), re-announce it, and play a hint-sound cue at the new level on priority group 0xb (survives the in-game Optionen pause), reusing the glossary preview handle.
- L648 `bool HandleInputList(int keyCode)`: Input for the option list.
- L744 `bool HandleInputGlossary(int keyCode)`
- L789 `bool HandleInput(int keyCode)`
- L843 `void Tick()`
- L878 `bool GetToggle(Option option)`

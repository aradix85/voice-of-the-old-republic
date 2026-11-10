# menus_modsettings.cpp (829 lines)

Source: `patches/Accessibility/menus_modsettings.cpp`

virtual mod-settings submenu.

See menus_modsettings.h for the design summary. This TU owns the sentinel pointer, the toggle bits, and the input router.

## Declarations (in source order)

- L196 `const OptionSpec& Spec(Option opt)`: Spec for `opt`.
- L227 `Option FocusedOption()`: Option under the cursor, or Option::Count when the index has gone out of range.
- L236 `const char* PersistKey(Option opt)`: Stable acc_settings.ini keys for the toggles that persist across launches.
- L250 `void EnsureTogglesLoaded()`: Pull persisted toggle values into s_toggles once, on first access.
- L278 `const char* StateText(Option opt)`
- L300 `void SpeakFocusedOption(bool interrupt = true)`: Speak the current focused option.
- L324 `void SpeakFocusedGlossaryEntry()`: Speak the current focused glossary entry — just the localised name.
- L336 `void CancelPendingGlossaryCue()`: Cancel any pending delayed glossary playback.
- L356 `bool ForegroundDivergedFromParent()`: True iff the engine pushed a new panel on top of whatever was foreground when the submenu opened — typically a MessageBoxModal raised by Alt+F4 / quit-confirm, but also any other engine modal the user hits while the s...
- L369 `bool IsRootAnchor(void* control)`
- L373 `void ForEachRootAnchor(void* panel, bool (*callback)(void* sentinel, int sortCx, int sortCy, void* userData), void* userData)`
- L398 `bool ExtractRootLabel(char* outBuf, size_t bufSize)`
- L412 `bool IsOpen()`
- L416 `void OpenSubMenu(void* parentPanel)`
- L440 `void Close()`
- L472 `void AutoCloseSilent()`: Silent auto-close used when the engine pushes a new panel (MessageBoxModal etc.) in front of our parent.
- L492 `void OpenGlossarySubMenu()`: Open the nested Audio glossary submenu and announce its title + first row.
- L510 `void CloseGlossarySubMenu()`: Close the glossary submenu, returning the user to the root.
- L535 `bool OpenExternalUrl(const char* url)`: Hand `url` to the default browser.
- L550 `void ActivateLinkRow(Option opt)`: Enter on a Link row: open the URL, then speak the outcome.
- L567 `void AdjustSlider(int delta)`: Adjust the cue-volume slider by delta (clamped to 0..100), re-announce it, and play a hint-sound cue at the new level on priority group 0xb (survives the in-game Optionen pause), reusing the glossary preview handle.
- L587 `bool HandleInputList(int keyCode)`: Input for the option list.
- L683 `bool HandleInputGlossary(int keyCode)`
- L728 `bool HandleInput(int keyCode)`
- L782 `void Tick()`
- L817 `bool GetToggle(Option option)`

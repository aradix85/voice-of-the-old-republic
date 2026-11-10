# menus_modsettings.cpp (834 lines)

Source: `patches/Accessibility/menus_modsettings.cpp`

virtual mod-settings submenu.

See menus_modsettings.h for the design summary. This TU owns the sentinel pointer, the toggle bits, and the input router.

## Declarations (in source order)

- L197 `const OptionSpec& Spec(Option opt)`: Spec for `opt`.
- L228 `Option FocusedOption()`: Option under the cursor, or Option::Count when the index has gone out of range.
- L237 `const char* PersistKey(Option opt)`: Stable acc_settings.ini keys for the toggles that persist across launches.
- L251 `void EnsureTogglesLoaded()`: Pull persisted toggle values into s_toggles once, on first access.
- L279 `const char* StateText(Option opt)`
- L301 `void SpeakFocusedOption(bool interrupt = true)`: Speak the current focused option.
- L325 `void SpeakFocusedGlossaryEntry()`: Speak the current focused glossary entry — just the localised name.
- L337 `void CancelPendingGlossaryCue()`: Cancel any pending delayed glossary playback.
- L357 `bool ForegroundDivergedFromParent()`: True iff the engine pushed a new panel on top of whatever was foreground when the submenu opened — typically a MessageBoxModal raised by Alt+F4 / quit-confirm, but also any other engine modal the user hits while the s...
- L370 `void* GetRootAnchor()`
- L374 `bool IsRootAnchor(void* control)`
- L378 `void ForEachRootAnchor(void* panel, bool (*callback)(void* sentinel, int sortCx, int sortCy, void* userData), void* userData)`
- L403 `bool ExtractRootLabel(char* outBuf, size_t bufSize)`
- L417 `bool IsOpen()`
- L421 `void OpenSubMenu(void* parentPanel)`
- L445 `void Close()`
- L477 `void AutoCloseSilent()`: Silent auto-close used when the engine pushes a new panel (MessageBoxModal etc.) in front of our parent.
- L497 `void OpenGlossarySubMenu()`: Open the nested Audio glossary submenu and announce its title + first row.
- L515 `void CloseGlossarySubMenu()`: Close the glossary submenu, returning the user to the root.
- L540 `bool OpenExternalUrl(const char* url)`: Hand `url` to the default browser.
- L555 `void ActivateLinkRow(Option opt)`: Enter on a Link row: open the URL, then speak the outcome.
- L572 `void AdjustSlider(int delta)`: Adjust the cue-volume slider by delta (clamped to 0..100), re-announce it, and play a hint-sound cue at the new level on priority group 0xb (survives the in-game Optionen pause), reusing the glossary preview handle.
- L592 `bool HandleInputList(int keyCode)`: Input for the option list.
- L688 `bool HandleInputGlossary(int keyCode)`
- L733 `bool HandleInput(int keyCode)`
- L787 `void Tick()`
- L822 `bool GetToggle(Option option)`

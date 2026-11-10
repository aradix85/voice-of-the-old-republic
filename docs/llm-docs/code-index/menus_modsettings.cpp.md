# menus_modsettings.cpp (864 lines)

Source: `patches/Accessibility/menus_modsettings.cpp`

virtual mod-settings submenu.

See menus_modsettings.h for the design summary. This TU owns the sentinel pointer, the toggle bits, and the input router.

## Declarations (in source order)

- L160 `const char* ReadmeLangCode(acc::strings::Lang l)`: ISO code used in the published readme filenames (docs/README.
- L176 `const char* BuildReadmeUrl(char* buf, size_t bufSize)`: Build the readme URL for the language the mod is currently speaking in.
- L226 `const OptionSpec& Spec(Option opt)`: Spec for `opt`.
- L257 `Option FocusedOption()`: Option under the cursor, or Option::Count when the index has gone out of range.
- L266 `const char* PersistKey(Option opt)`: Stable acc_settings.ini keys for the toggles that persist across launches.
- L280 `void EnsureTogglesLoaded()`: Pull persisted toggle values into s_toggles once, on first access.
- L308 `const char* StateText(Option opt)`
- L330 `void SpeakFocusedOption(bool interrupt = true)`: Speak the current focused option.
- L354 `void SpeakFocusedGlossaryEntry()`: Speak the current focused glossary entry — just the localised name.
- L366 `void CancelPendingGlossaryCue()`: Cancel any pending delayed glossary playback.
- L386 `bool ForegroundDivergedFromParent()`: True iff the engine pushed a new panel on top of whatever was foreground when the submenu opened — typically a MessageBoxModal raised by Alt+F4 / quit-confirm, but also any other engine modal the user hits while the s...
- L399 `void* GetRootAnchor()`
- L403 `bool IsRootAnchor(void* control)`
- L407 `void ForEachRootAnchor(void* panel, bool (*callback)(void* sentinel, int sortCx, int sortCy, void* userData), void* userData)`
- L432 `bool ExtractRootLabel(char* outBuf, size_t bufSize)`
- L446 `bool IsOpen()`
- L450 `void OpenSubMenu(void* parentPanel)`
- L474 `void Close()`
- L506 `void AutoCloseSilent()`: Silent auto-close used when the engine pushes a new panel (MessageBoxModal etc.) in front of our parent.
- L526 `void OpenGlossarySubMenu()`: Open the nested Audio glossary submenu and announce its title + first row.
- L544 `void CloseGlossarySubMenu()`: Close the glossary submenu, returning the user to the root.
- L569 `bool OpenExternalUrl(const char* url)`: Hand `url` to the default browser.
- L584 `void ActivateLinkRow(Option opt)`: Enter on a Link row: open the URL, then speak the outcome.
- L602 `void AdjustSlider(int delta)`: Adjust the cue-volume slider by delta (clamped to 0..100), re-announce it, and play a hint-sound cue at the new level on priority group 0xb (survives the in-game Optionen pause), reusing the glossary preview handle.
- L622 `bool HandleInputList(int keyCode)`: Input for the option list.
- L718 `bool HandleInputGlossary(int keyCode)`
- L763 `bool HandleInput(int keyCode)`
- L817 `void Tick()`
- L852 `bool GetToggle(Option option)`

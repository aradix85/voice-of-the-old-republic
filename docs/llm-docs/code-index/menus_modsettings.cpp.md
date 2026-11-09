# menus_modsettings.cpp (1329 lines)

Source: `patches/Accessibility/menus_modsettings.cpp`

virtual mod-settings submenu.

See menus_modsettings.h for the design summary. This TU owns the sentinel pointer, the toggle bits, and the input router.

## Declarations (in source order)

- L214 `const char* ReadmeLangCode(acc::strings::Lang l)`: ISO code used in the published readme filenames (docs/README.
- L230 `const char* BuildReadmeUrl(char* buf, size_t bufSize)`: Build the readme URL for the language the mod is currently speaking in.
- L262 `int GetSliderValue(Option opt)`
- L278 `void SetSliderValue(Option opt, int value)`
- L295 `bool IsRateOffsetSlider(Option opt)`: True for the combat speed rows, whose value is an offset, not a percent.
- L300 `bool CombatSliderChannel(Option opt, Channel& out)`: The positional channel a combat slider tunes (for its preview), or false.
- L311 `void SliderRange(Option opt, int& lo, int& hi)`: The range a Slider row clamps to.
- L324 `int SliderStep(Option opt)`: Amount a single Left/Right press moves the given Slider row by.
- L377 `const OptionSpec& Spec(Option opt)`: Spec for `opt`.
- L476 `const NestedView* NestedViewFor(Option pivot)`: The nested view in front, or nullptr on the root list.
- L489 `RowView CurrentView()`
- L494 `int& CurrentFocus()`
- L500 `Option FocusedOption()`: Option under the cursor in the active view, or Option::Count when the view is empty or the index has gone out of range.
- L511 `const char* PersistKey(Option opt)`: Stable acc_settings.ini keys for the toggles that persist across launches.
- L530 `void EnsureTogglesLoaded()`: Pull persisted toggle values into s_toggles once, on first access.
- L582 `size_t VoiceCount(Option opt)`: The two voice-choice rows: which SAPI voice the announcement channel (UrgentVoice) or the compass channel (CompassVoice) speaks with.
- L586 `size_t GetVoiceId(Option opt)`
- L590 `void SetVoiceId(Option opt, size_t id)`
- L595 `bool IsCompassRow(Option opt)`
- L604 `void SpeakSample(Option opt)`: Sample line for a voice or slider row, spoken on the channel that row tunes so it comes out in the voice, loudness and pace just chosen.
- L619 `const char* VoiceText(Option opt)`: Value text for a voice-choice row: the voice's name.
- L629 `const char* StateText(Option opt)`
- L659 `void SpeakFocusedOption(bool interrupt = true)`: Speak the current focused option.
- L699 `void SpeakFocusedGlossaryEntry()`: Speak the current focused glossary entry — just the localised name.
- L711 `void CancelPendingGlossaryCue()`: Cancel any pending delayed glossary playback.
- L731 `bool ForegroundDivergedFromParent()`: True iff the engine pushed a new panel on top of whatever was foreground when the submenu opened — typically a MessageBoxModal raised by Alt+F4 / quit-confirm, but also any other engine modal the user hits while the s...
- L744 `void* GetRootAnchor()`
- L748 `bool IsRootAnchor(void* control)`
- L752 `void ForEachRootAnchor(void* panel, bool (*callback)(void* sentinel, int sortCx, int sortCy, void* userData), void* userData)`
- L777 `bool ExtractRootLabel(char* outBuf, size_t bufSize)`
- L791 `bool IsOpen()`
- L795 `void OpenSubMenu(void* parentPanel)`
- L824 `void Close()`
- L859 `void AutoCloseSilent()`: Silent auto-close used when the engine pushes a new panel (MessageBoxModal etc.) in front of our parent.
- L882 `void OpenGlossarySubMenu()`: Open the nested Audio glossary submenu and announce its title + first row.
- L900 `void CloseGlossarySubMenu()`: Close the glossary submenu, returning the user to the root.
- L919 `void OpenNestedView(const NestedView& view)`: Open a nested list view (announcement voice, compass voice, combat announcements) and announce its title + first row.
- L931 `void CloseNestedView()`: Close it, returning the user to the view it was opened from.
- L951 `bool OpenExternalUrl(const char* url)`: Hand `url` to the default browser.
- L966 `void ActivateLinkRow(Option opt)`: Enter on a Link row: open the URL, then speak the outcome.
- L998 `void AdjustSlider(int delta)`: Adjust the focused Slider row by delta (clamped to its range), re-announce it, and play an audible preview at the new setting so the user hears what they just dialled in.
- L1044 `void AdjustVoice(int delta)`: Step the focused voice row (announcement or compass voice) by `delta` places in Prism's SAPI voice list, clamped at both ends (no wrap — same rule as the row ladder, and the repeated name is the boundary cue).
- L1068 `bool HandleInputList(int keyCode)`: Input for whichever option list is in front — the root list, or the announcement-voice submenu.
- L1181 `bool HandleInputGlossary(int keyCode)`
- L1226 `bool HandleInput(int keyCode)`
- L1280 `void Tick()`
- L1317 `bool GetToggle(Option option)`

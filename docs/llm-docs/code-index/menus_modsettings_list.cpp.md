# menus_modsettings_list.cpp (202 lines)

Source: `patches/Accessibility/menus_modsettings_list.cpp`

Input on the option list: toggles, the cue-volume slider, the submenu pivots and the link rows.

## Declarations (in source order)

- L51 `bool OpenExternalUrl(const char* url)`: Hand `url` to the default browser.
- L66 `void ActivateLinkRow(Option opt)`: Enter on a Link row: open the URL, then speak the outcome.
- L83 `void AdjustSlider(int delta)`: Adjust the cue-volume slider by delta (clamped to 0..100), re-announce it, and play a hint-sound cue at the new level on priority group 0xb (survives the in-game Options pause), reusing the glossary preview handle.
- L105 `bool HandleInputList(int keyCode)`: Input for the option list.

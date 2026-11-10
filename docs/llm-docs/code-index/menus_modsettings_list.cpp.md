# menus_modsettings_list.cpp (152 lines)

Source: `patches/Accessibility/menus_modsettings_list.cpp`

Input on the option list: toggles, the cue-volume slider, the submenu pivots and the link rows.

## Declarations (in source order)

- L36 `bool OpenExternalUrl(const char* url)`: Hand `url` to the default browser; false when the shell refused.
- L45 `void ActivateLinkRow(Option opt)`: The submenu stays open, so the user is on the same row after the browser.
- L56 `void AdjustSlider(int delta)`: Step the cue volume (clamped to 0..100), say it, and preview the new level.
- L76 `void ToggleSkipIntros()`: SkipIntros renames the intro movie instead of flipping a bit; the change applies on the next launch.
- L89 `void FlipToggle(Option opt)`
- L99 `void Activate(Option opt)`
- L127 `bool HandleInputList(int keyCode)`

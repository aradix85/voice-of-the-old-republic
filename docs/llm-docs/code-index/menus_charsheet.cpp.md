# menus_charsheet.cpp (292 lines)

Source: `patches/Accessibility/menus_charsheet.cpp`

character sheet stat rows.

See menus_charsheet.h for design overview. User-facing strings route through acc::strings::Get (memory: feedback_centralise_user_strings.md) so a non-German install gets localised phrases.

## Declarations (in source order)

- L104 `bool DisplayedHasForce(void* panel)`: True iff the character currently shown on this sheet is a Force user.
- L179 `const StatRowSpec* FindSpecForControl(void* panel, void* labelControl)`: Resolve `labelControl` to a StatRowSpec for `panel`.
- L204 `bool IsStatRowAnchor(void* panel, void* labelControl)`
- L208 `void ForEachStatRowAnchor(void* panel, bool (*callback)(void* labelControl, int sortCy, void* userData), void* userData)`
- L231 `bool ExtractStatRow(void* panel, void* labelControl, char* outBuf, size_t bufSize)`

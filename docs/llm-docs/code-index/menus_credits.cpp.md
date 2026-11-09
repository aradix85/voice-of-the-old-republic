# menus_credits.cpp (179 lines)

Source: `patches/Accessibility/menus_credits.cpp`

virtual credits row (Inventory + Store).

See menus_credits.h for the design summary. This file owns the per-kind anchor table and the read+format path.

## Declarations (in source order)

- L83 `const CreditsAnchorSpec* FindSpecForPanel(void* panel)`
- L96 `void* AnchorLabel(void* panel, const CreditsAnchorSpec* spec)`: The value label this spec anchors on, or null.
- L109 `bool IsCreditsRowAnchor(void* panel, void* labelControl)`
- L115 `void ForEachCreditsRowAnchor(void* panel, bool (*callback)(void* labelControl, int sortCy, void* userData), void* userData)`
- L132 `bool ExtractCreditsRow(void* panel, void* labelControl, char* outBuf, size_t bufSize)`

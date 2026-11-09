# menus_extract_chargen.cpp (422 lines)

Source: `patches/Accessibility/menus_extract_chargen.cpp`

Character creation: the class selection icons and the portrait arrows (with the portrait variants in portraits.2da order).

## Declarations (in source order)

- L76 `int PortraitVariantRank(uint32_t row)`: 1-based position of `row` inside its own (gender, family) group, or 0 when the row isn't one this game's table knows.
- L138 `const char* TryClassSelectionIcon(void* control, void* owner, char* outBuf, size_t bufSize)`: 9c.
- L232 `const char* TryPortraitCharGenArrow(void* control, void* owner, char* outBuf, size_t bufSize)`: 9d.

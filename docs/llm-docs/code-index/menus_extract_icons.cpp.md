# menus_extract_icons.cpp (302 lines)

Source: `patches/Accessibility/menus_extract_icons.cpp`

Icon controls of the in-game screens: party portraits, the speculative read of an unknown control's text, the in-game menu icons, equipment slots and the map arrows.

## Declarations (in source order)

- L43 `const char* TryPartyPortrait(void* control, char* outBuf, size_t bufSize, bool& claimedVtable)`: 7b.
- L146 `const char* TryInGameMenuIcon(void* control, void* owner, char* outBuf, size_t bufSize)`: 9a.
- L273 `const char* TryInGameMapArrow(void* control, void* owner, char* outBuf, size_t bufSize)`: 9b2.

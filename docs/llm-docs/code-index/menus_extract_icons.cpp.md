# menus_extract_icons.cpp (636 lines)

Source: `patches/Accessibility/menus_extract_icons.cpp`

Icon controls of the in-game screens: party portraits, the speculative read of an unknown control's text, the in-game menu icons, equipment slots and the map arrows.

## Declarations (in source order)

- L103 `const char* TryPartyPortrait(void* control, char* outBuf, size_t bufSize, bool& claimedVtable)`: 7b.
- L201 `const char* TrySpeculativeVtableRead(void* control, char* outBuf, size_t bufSize)`: 8.
- L364 `const char* TryInGameMenuIcon(void* control, void* owner, char* outBuf, size_t bufSize)`: 9a.
- L474 `const char* TryEquipSlot(void* control, void* owner, char* outBuf, size_t bufSize)`
- L585 `const char* TryInGameMapArrow(void* control, void* owner, char* outBuf, size_t bufSize)`: 9b2.
- L613 `bool ReadEquipSlotItemName(void* panel, void* slotBtn, char* outBuf, size_t bufSize)`

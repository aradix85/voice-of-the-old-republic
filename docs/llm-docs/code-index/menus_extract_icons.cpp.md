# menus_extract_icons.cpp (629 lines)

Source: `patches/Accessibility/menus_extract_icons.cpp`

Icon controls of the in-game screens: party portraits, the speculative read of an unknown control's text, the in-game menu icons, equipment slots and the map arrows.

## Declarations (in source order)

- L94 `const char* TryPartyPortrait(void* control, char* outBuf, size_t bufSize, bool& claimedVtable)`: 7b.
- L192 `const char* TrySpeculativeVtableRead(void* control, char* outBuf, size_t bufSize)`: 8.
- L355 `const char* TryInGameMenuIcon(void* control, void* owner, char* outBuf, size_t bufSize)`: 9a.
- L467 `const char* TryEquipSlot(void* control, void* owner, char* outBuf, size_t bufSize)`
- L578 `const char* TryInGameMapArrow(void* control, void* owner, char* outBuf, size_t bufSize)`: 9b2.
- L606 `bool ReadEquipSlotItemName(void* panel, void* slotBtn, char* outBuf, size_t bufSize)`

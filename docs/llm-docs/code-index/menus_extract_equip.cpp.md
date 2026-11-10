# menus_extract_equip.cpp (117 lines)

Source: `patches/Accessibility/menus_extract_equip.cpp`

The equipment screen's slot buttons: the slot name (TLK first, the string table as fallback, the K2 weapon-set qualifier) and the equipped item.

## Declarations (in source order)

- L52 `bool SlotLabel(int idx, char* out, size_t size)`
- L75 `bool SlotItemName(void* panel, int idx, char* out, size_t size)`: The panel's per-slot handles are filled from the displayed character's inventory (character cycling rewrites them), so they match the screen.
- L90 `const char* TryEquipSlot(void* control, void* owner, char* outBuf, size_t bufSize)`
- L109 `bool ReadEquipSlotItemName(void* panel, void* slotBtn, char* outBuf, size_t bufSize)`

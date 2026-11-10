# menus_extract_equip.cpp (193 lines)

Source: `patches/Accessibility/menus_extract_equip.cpp`

The equipment screen's slot buttons: the slot name (TLK first, string table as fallback, the K2 weapon-set qualifier) and the equipped item.

## Declarations (in source order)

- L76 `const char* TryEquipSlot(void* control, void* owner, char* outBuf, size_t bufSize)`
- L170 `bool ReadEquipSlotItemName(void* panel, void* slotBtn, char* outBuf, size_t bufSize)`

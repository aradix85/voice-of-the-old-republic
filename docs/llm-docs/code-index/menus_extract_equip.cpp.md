# menus_extract_equip.cpp (115 lines)

Source: `patches/Accessibility/menus_extract_equip.cpp`

The equipment screen's slot buttons: the slot name (TLK first, the string table as fallback, the K2 weapon-set qualifier) and the equipped item.

## Declarations (in source order)

- L50 `bool SlotLabel(int idx, char* out, size_t size)`
- L73 `bool SlotItemName(void* panel, int idx, char* out, size_t size)`: The panel's per-slot handles are filled from the displayed character's inventory (character cycling rewrites them), so they match the screen.
- L88 `const char* TryEquipSlot(void* control, void* owner, char* outBuf, size_t bufSize)`
- L107 `bool ReadEquipSlotItemName(void* panel, void* slotBtn, char* outBuf, size_t bufSize)`

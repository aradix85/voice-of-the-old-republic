# menus_listbox_equip.cpp (178 lines)

Source: `patches/Accessibility/menus_listbox_equip.cpp`

The equipment picker: the list of items for a slot, equipping on Enter.

## Declarations (in source order)

- L15 `bool EquipPickerMatchesPanel(void* p)`
- L19 `bool EquipPickerArmed()`
- L27 `void* EquipPickerFindLb(void* p)`
- L38 `void EquipPickerAnnounce(void* /*lb*/, const ListBoxNavResult& r)`: Inline announce only on no-op clamp; normal moves are caught by MonitorEquipPickerSelection (menus_listbox_picker.cpp).
- L51 `void EquipPickerLogExtra(char* out, size_t outN, const ListBoxNavResult& r)`
- L58 `bool EquipPickerOnEnter(void* panel)`: Custom Enter: lookup row + btn from listbox.selection_index, queue an EquipCommit op, disarm.
- L146 `bool EquipPickerOnEsc(void* panel)`: Custom Esc: run the ENGINE's cancel, then drop our latch.

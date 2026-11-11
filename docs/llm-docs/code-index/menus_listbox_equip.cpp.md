# menus_listbox_equip.cpp (171 lines)

Source: `patches/Accessibility/menus_listbox_equip.cpp`

The equipment picker: the list of items for a slot, equipping on Enter.

## Declarations (in source order)

- L23 `bool EquipPickerMatchesPanel(void* p)`
- L27 `bool EquipPickerArmed()`
- L35 `void* EquipPickerFindLb(void* p)`
- L46 `void EquipPickerAnnounce(void* /*lb*/, const ListBoxNavResult& r)`: Inline announce only on no-op clamp; normal moves are caught by MonitorEquipPickerSelection (menus_listbox_picker.cpp).
- L59 `void EquipPickerLogExtra(char* out, size_t outN, const ListBoxNavResult& r)`
- L66 `bool EquipPickerOnEnter(void* panel)`: Custom Enter: lookup row + btn from listbox.selection_index, queue an EquipCommit op, disarm.
- L139 `bool EquipPickerOnEsc(void* panel)`: Custom Esc: run the ENGINE's cancel, then drop our latch.

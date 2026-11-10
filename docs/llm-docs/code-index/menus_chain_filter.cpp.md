# menus_chain_filter.cpp (322 lines)

Source: `patches/Accessibility/menus_chain_filter.cpp`

Which controls stay out of the chain: decorative labels and buttons, and the per-panel controls that are reached another way.

## Declarations (in source order)

- L26 `void* ResolvePortraitChargenSkip(void* panel)`: Panel-aware chain filter: in CSWGuiPortraitCharGen we anchor the chain on the left_arrow alone (announces the current portrait via the PerKind path in menus_extract.cpp's section 9d) and consolidate the right_arrow ou...
- L40 `void ResolveStoreActionButtons(void* panel, void*& storeCancelBtn, void*& storeToggleBtn, void*& storeAcceptBtn)`: Store-panel action buttons (Close / Sell / Buy).
- L61 `void* ResolveEquipPickerListBox(void* panel)`: InGameEquip picker listbox (LB_ITEMS, id=5): the equip panel keeps the picker's item listbox in panel.controls[] even when the picker isn't visually shown — the engine pre-populates it with the body- slot candidates a...
- L78 `void* ResolveWorkbenchUpgradeListBox(void* panel)`: WorkbenchUpgrade LB_ITEMS (id=0): the compatible-crystal list.
- L111 `bool IsDecorativeControl(void* panel, void* c, const char* closeCaption, bool haveCloseCaption)`: Per-kind decorative filter.

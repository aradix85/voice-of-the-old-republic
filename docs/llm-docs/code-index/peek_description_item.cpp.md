# peek_description_item.cpp (439 lines)

Source: `patches/Accessibility/peek_description_item.cpp`

Reading an item's description block by block: the equipment and workbench slot tooltips, the item rows of the item screens, and the workbench saber description.

## Declarations (in source order)

- L30 `bool IsBlank(const char* s)`
- L39 `void TrimCopy(char* dst, std::size_t cap, const char* src)`: Copy src into dst with leading/trailing whitespace stripped (the engine builders pad each block with a trailing "\n\n").
- L55 `void* ContainerFindLb(void* panel)`: Container / equip / workbench-upgrade listboxes all resolve through the shared engine-member resolvers (menus_internal.cpp) — the same ctor-mined offsets this file used to duplicate locally (docs/gui-id-audit.md).
- L59 `void* InGameEquipFindLb(void* panel)`
- L63 `void* WorkbenchItemsFindLb(void* panel)`
- L67 `void* WorkbenchUpgradeFindLb(void* panel)`
- L78 `bool TryReadWorkbenchSaberDescription(void* panel, void* row, char* outBuf, std::size_t bufSize)`: Workbench saber-upgrade picker: build the description the way the engine's own hover handler (CSWGuiUpgrade::OnControlEntered) does, so the keyed bonus line it prepends — which the generic GetPropertyDescription path ...
- L132 `bool SpeakItemBlocks(void* item, bool down)`: Shift+Up/Down over an item's four categorised description blocks.
- L188 `bool HandleEquipSlotTooltip(void* panel, int slotIdx, bool down)`: Per-slot peek for the equip slot buttons (all 9, plus KOTOR 2's two second-weapon-set slots for free).
- L232 `bool HandleWorkbenchSlotTooltip(void* panel, void* control, bool down)`: Shift+arrow on a workbench upgrade slot button (upgrade.gui IDs 12..18): speak the installed mod's full property description ("what the slot does for your item"), or announce the slot is empty.
- L288 `const ItemTooltipPanelInfo* LookupItemTooltipPanel(acc::engine::PanelKind k)`
- L298 `void* ResolveRowItem(void* row)`: Read an item entry row's client handle (+0x1c4) and resolve it to a CSWSItem*.
- L312 `bool HandleItemTooltip(acc::engine::PanelKind kind, const ItemTooltipPanelInfo& info, void* activePanel, bool down)`: Caller consumes the key regardless of return (predictable behaviour).
- L409 `bool SpeakItemRowDescription(void* row)`

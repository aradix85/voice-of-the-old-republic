# peek_description.cpp (847 lines)

Source: `patches/Accessibility/peek_description.cpp`

(From peek_description.h.) Shift+Up/Down description peek.

Shift+arrow speaks one description block at a time from the focused panel's description listbox (Inventory, Journal, Abilities, Store) or from per-item tooltip resolution (Container, Equip picker, Workbench) or the focused control's tooltip strref as a generic fallback. Shift release resets the block cursor.

Adding a new panel = one entry in the panel registry inside the .cpp (offset of the description_listbox member, optional refresh adapter).

## Declarations (in source order)

- L44 `bool IsBlank(const char* s)`
- L53 `void TrimCopy(char* dst, std::size_t cap, const char* src)`: Copy src into dst with leading/trailing whitespace stripped (the engine builders pad each block with a trailing "\n\n").
- L73 `bool SpeakItemBlocks(void* item, bool down)`: Shift+Up/Down over an item's four categorised description blocks.
- L124 `static void CallOnControlEnteredWithActive(std::uintptr_t addr, void* panel, void* focused)`: Several OnControlEntered overrides early-out on `entry->is_active != 0`.
- L154 `static void RefreshInventory(void* panel, void* focused)`
- L166 `static void RefreshStore(void* panel, void* focused)`
- L177 `static void RefreshJournal(void* panel, void* focused)`
- L193 `static void RefreshAbilities(void* panel, void* /*focused*/)`: Repaint LB_DESC for the focused entry before Shift+Up/Down pages it.
- L232 `const PanelPeekInfo* LookupPanel(acc::engine::PanelKind k)`
- L263 `void* ContainerFindLb(void* panel)`: Container / equip / workbench-upgrade listboxes all resolve through the shared engine-member resolvers (menus_internal.cpp) — the same ctor-mined offsets this file used to duplicate locally (docs/gui-id-audit.md).
- L267 `void* InGameEquipFindLb(void* panel)`
- L271 `void* WorkbenchItemsFindLb(void* panel)`
- L275 `void* WorkbenchUpgradeFindLb(void* panel)`
- L309 `bool HandleEquipSlotTooltip(void* panel, int slotIdx, bool down)`: Per-slot peek for the equip slot buttons (all 9, plus KOTOR 2's two second-weapon-set slots for free).
- L353 `bool HandleWorkbenchSlotTooltip(void* panel, void* control, bool down)`: Shift+arrow on a workbench upgrade slot button (upgrade.gui IDs 12..18): speak the installed mod's full property description ("what the slot does for your item"), or announce the slot is empty.
- L390 `const ItemTooltipPanelInfo* LookupItemTooltipPanel(acc::engine::PanelKind k)`
- L407 `void* ResolveRowItem(void* row)`: Read an item entry row's client handle (+0x1c4) and resolve it to a CSWSItem*.
- L426 `bool HandleItemTooltip(acc::engine::PanelKind kind, const ItemTooltipPanelInfo& info, void* activePanel, bool down)`: Caller consumes the key regardless of return (predictable behaviour).
- L525 `const char* ReadRowText(void* row, char* outBuf, std::size_t bufSize)`: Rows can be CSWGuiLabel OR CSWGuiButton (engine uses both for text-only containers).
- L549 `bool ShiftHeld()`
- L568 `bool TryReadWorkbenchSaberDescription(void* panel, void* row, char* outBuf, std::size_t bufSize)`: Workbench saber-upgrade picker: build the description the way the engine's own hover handler (CSWGuiUpgrade::OnControlEntered) does, so the keyed bonus line it prepends — which the generic GetPropertyDescription path ...
- L616 `void OnShiftReleased()`
- L628 `bool SpeakItemRowDescription(void* row)`
- L657 `bool HandleShiftArrow(int param_1, int param_2, void* activePanel, void* focusedControl)`

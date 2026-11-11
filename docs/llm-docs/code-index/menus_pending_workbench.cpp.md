# menus_pending_workbench.cpp (289 lines)

Source: `patches/Accessibility/menus_pending_workbench.cpp`

The workbench's deferred ops: selecting an upgrade slot, installing or removing an upgrade (two steps on KOTOR 1, one on KOTOR 2), and closing the mod picker.

## Declarations (in source order)

- L27 `void* ReadUpgradeSlotInstalled(void* panel)`: K2: the installed-mod pointer (field35) of the currently-open upgrade slot.
- L45 `bool IsUpgradePickerNoneRow(void* row)`: K2: true iff `row` is the engine's "-" none/remove picker entry (key -1).
- L51 `int UpgradeItemListRows(void* panel)`: Rows in the panel's LB_ITEMS (the control with id 0).
- L68 `void AnnounceSlotOutcome(int slotIdx, int saberFlag, void* prevItem, void* newItem)`: The non-saber slot click installed, removed or found nothing: say so, with the slot's type name from the engine's strref table, so the user hears WHICH slot they acted on rather than a position number.
- L118 `void DrainWorkbenchSlotSelect(const PendingOp& op)`: Workbench slot activation.
- L183 `void DrainWorkbenchUpgradeCommit(const PendingOp& op)`: Workbench upgrade commit.
- L221 `void DrainWorkbenchUpgradeInstallK2(const PendingOp& op)`: K2 upgrade install/remove — ONE step.
- L272 `void DrainWorkbenchPickerCancel(const PendingOp& op)`: Back out of the mod-picker without committing (Esc).

# menus_pending_workbench.cpp (296 lines)

Source: `patches/Accessibility/menus_pending_workbench.cpp`

The workbench's deferred ops: selecting an upgrade slot, installing or removing an upgrade (two steps on KOTOR 1, one on KOTOR 2), and closing the mod picker.

## Declarations (in source order)

- L25 `void* ReadUpgradeSlotInstalled(void* panel)`: K2: the installed-mod pointer (field35) of the currently-open upgrade slot.
- L43 `bool IsUpgradePickerNoneRow(void* row)`: K2: true iff `row` is the engine's "-" none/remove picker entry (key -1).
- L63 `void DrainWorkbenchSlotSelect(const PendingOp& op)`: Workbench slot activation.
- L188 `void DrainWorkbenchUpgradeCommit(const PendingOp& op)`: Workbench upgrade commit.
- L228 `void DrainWorkbenchUpgradeInstallK2(const PendingOp& op)`: K2 upgrade install/remove — ONE step.
- L279 `void DrainWorkbenchPickerCancel(const PendingOp& op)`: Back out of the mod-picker without committing (Esc).

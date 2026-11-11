# menus_pending_equip.cpp (111 lines)

Source: `patches/Accessibility/menus_pending_equip.cpp`

The equipment screen's deferred ops: opening a slot's item picker, equipping the chosen row, and the engine's own cancel.

## Declarations (in source order)

- L27 `void DrainEquipSelect(const PendingOp& op)`: Equip-screen slot activation.
- L52 `void DrainEquipCommit(const PendingOp& op)`: Equip-row commit.
- L98 `void DrainEquipPickerCancel(const PendingOp& op)`: Equip-picker cancel — the engine's own "back out of the item zone".

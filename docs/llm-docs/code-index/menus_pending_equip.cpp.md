# menus_pending_equip.cpp (110 lines)

Source: `patches/Accessibility/menus_pending_equip.cpp`

The equipment screen's deferred ops: opening a slot's item picker, equipping the chosen row, and the engine's own cancel.

## Declarations (in source order)

- L26 `void DrainEquipSelect(const PendingOp& op)`: Equip-screen slot activation.
- L51 `void DrainEquipCommit(const PendingOp& op)`: Equip-row commit.
- L97 `void DrainEquipPickerCancel(const PendingOp& op)`: Equip-picker cancel — the engine's own "back out of the item zone".

# menus_pending_internal.h (68 lines)

Source: `patches/Accessibility/menus_pending_internal.h`

Internal to the menus_pending module (menus_pending*.cpp); no other file includes it. The public interface is menus_pending.h. Here: what the files share.

## Declarations (in source order)

- L14 `enum class Kind`
- L36 `struct PendingOp`
- L47 `inline const char* RaiseNote(uint32_t prevIsActive)`: The log note for a RaiseIsActiveIfZero result.
- L54 `void DrainActivate(const PendingOp& op)`: menus_pending_activate.cpp
- L57 `void DrainEquipSelect(const PendingOp& op)`: menus_pending_equip.cpp
- L58 `void DrainEquipCommit(const PendingOp& op)`
- L59 `void DrainEquipPickerCancel(const PendingOp& op)`
- L62 `void DrainWorkbenchSlotSelect(const PendingOp& op)`: menus_pending_workbench.cpp
- L63 `void DrainWorkbenchUpgradeCommit(const PendingOp& op)`
- L64 `void DrainWorkbenchUpgradeInstallK2(const PendingOp& op)`
- L65 `void DrainWorkbenchPickerCancel(const PendingOp& op)`

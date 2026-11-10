# menus_pending_internal.h (70 lines)

Source: `patches/Accessibility/menus_pending_internal.h`

Internal to the menus_pending module (menus_pending*.cpp); no other file includes it. The public interface is menus_pending.h. Here: what the files share.

## Declarations (in source order)

- L15 `enum class Kind`
- L37 `struct PendingOp`
- L51 `typedef void (__thiscall* PFN_ControlHandleInputEvent)(void* this_, int code, int state)`
- L56 `void DrainActivate(const PendingOp& op)`: menus_pending_activate.cpp
- L59 `void DrainEquipSelect(const PendingOp& op)`: menus_pending_equip.cpp
- L60 `void DrainEquipCommit(const PendingOp& op)`
- L61 `void DrainEquipPickerCancel(const PendingOp& op)`
- L64 `void DrainWorkbenchSlotSelect(const PendingOp& op)`: menus_pending_workbench.cpp
- L65 `void DrainWorkbenchUpgradeCommit(const PendingOp& op)`
- L66 `void DrainWorkbenchUpgradeInstallK2(const PendingOp& op)`
- L67 `void DrainWorkbenchPickerCancel(const PendingOp& op)`

Also 1 constants or variables at namespace scope.

# menus_pending_internal.h (80 lines)

Source: `patches/Accessibility/menus_pending_internal.h`

Internal to the menus_pending module (menus_pending*.cpp); no other file includes it. The public interface is menus_pending.h. Here: what the files share.

## Declarations (in source order)

- L14 `enum class Kind`
- L36 `struct PendingOp`
- L46 `typedef void (__thiscall* PFN_ControlHandleInputEvent)(void* this_, int code, int state)`
- L50 `inline PFN_ControlHandleInputEvent HandleInputEventOf(void* obj)`: A control's or panel's own HandleInputEvent (vtable slot 15); nullptr when the object or its vtable is missing.
- L59 `inline const char* RaiseNote(uint32_t prevIsActive)`: The log note for a RaiseIsActiveIfZero result.
- L66 `void DrainActivate(const PendingOp& op)`: menus_pending_activate.cpp
- L69 `void DrainEquipSelect(const PendingOp& op)`: menus_pending_equip.cpp
- L70 `void DrainEquipCommit(const PendingOp& op)`
- L71 `void DrainEquipPickerCancel(const PendingOp& op)`
- L74 `void DrainWorkbenchSlotSelect(const PendingOp& op)`: menus_pending_workbench.cpp
- L75 `void DrainWorkbenchUpgradeCommit(const PendingOp& op)`
- L76 `void DrainWorkbenchUpgradeInstallK2(const PendingOp& op)`
- L77 `void DrainWorkbenchPickerCancel(const PendingOp& op)`

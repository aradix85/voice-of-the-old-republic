# menus_listbox.h (124 lines)

Source: `patches/Accessibility/menus_listbox.h`

listbox-driven panel input dispatcher.

(Container loot, SaveLoad, EquipPicker item-pick) share a "select-then- confirm" interaction shape: arrow keys drive a listbox cursor we announce inline, Enter dispatches a panel-specific commit, Esc backs out. Each had its own ~80-120 line block in OnHandleInputEvent.

This TU collapses those three blocks into a spec-table-driven dispatcher. Each panel is described by a `ListBoxPanelSpec` value: how to match the panel, when it's armed, where the listbox lives, how to announce a row, what Enter and Esc do, whether to fall through if nothing was consumed. Adding a 4th similar panel is one new spec entry plus its callbacks (~30 lines), not another copy of the handler scaffolding.

Why a spec table (not just a helper that consolidates the duplicated listbox-arrow nav): the divergence isn't decorative — Container's Enter is QueueButtonByIdActivate(BTN_OK), SaveLoad's is QueueButtonByIdActivate(saveload_button), EquipPicker's is a custom row commit (QueueEquipCommit + disarm). The spec encodes that variation as onEnter / onEsc callbacks, keeping the dispatcher generic.

EquipPicker state (armed flag + bound panel) lives in this TU because the picker's input handler is the primary owner. The slot-Enter arming site (menus_chain_input_enter.cpp) and MonitorEquipPickerSelection's "panel gone, disarm" cleanup (menus_listbox_picker.cpp) call the accessors below.

## Declarations (in source order)

- L47 `bool TryHandleInput(int n, void* thisPtr, void* activePanel, int param_1, int param_2, int& outRv)`: Try to handle the input event against one of the listbox-driven panel specs.
- L61 `const char* GetTitleOverride(void* panel)`: Title-speech override lookup.
- L86 `bool IsEquipPickerArmed()`: EquipPicker zone state.
- L87 `void ArmEquipPicker(void* panel, void* slotBtn)`
- L88 `void ClearEquipPickerArmLatch()`
- L99 `bool IsWorkbenchUpgradePickerArmed()`: Workbench upgrade picker — arms when the chain Enter handler activates a BTN_UPGRADE3X/4X slot button (populating LB_ITEMS with the slot's compatible inventory mods).
- L100 `void* WorkbenchUpgradePickerPanel()`
- L101 `void ArmWorkbenchUpgradePicker(void* panel)`
- L102 `void ClearWorkbenchUpgradeArmLatch()`
- L110 `void TickListboxMonitors()`: Per-tick fan-out for the listbox-paired monitors that live in this TU: MonitorContainerSelection (per-row navigation announces) and PollContainerGiveModeKey (Win32 poll for the give-mode toggle key — the engine's play...
- L115 `void TickPickerMonitors()`: The two armed-picker monitors (equip + workbench upgrade), in menus_listbox_picker.cpp.
- L121 `void RegisterPickerMsgRules()`: Registers the picker-owned message-buffer rules.

# menus_pending.h (188 lines)

Source: `patches/Accessibility/menus_pending.h`

deferred menu-side operation queue.

g_pendingActivate, g_pendingCursorMove, g_pendingSliderInput, g_pendingEquipSelect, g_pendingEquipCommit) flag/target pairs collapse to a single tagged op stored here. Input handlers in menus.cpp call `Queue*` to enqueue; `Drain` runs once per tick from `TickPendingOps` and dispatches the op against deep engine functions (MoveMouseToPosition, manager LMouseDown/Up, vtable[15] HandleInputEvent, OnEnterSlot/ OnSelectSlot, OnItemSelected/OnOKPressed).

Why deferred: invoking these mid-input-dispatch recurses through HandleMouseMove → UpdateMouseOverControl, the same re-entrancy class that destabilised earlier listbox-entry hooks. Deferring to the next CSWGuiManager::Update tick (~16ms at 60fps; inaudible) keeps deep engine re-entry off the input-hook stack. Speech still fires synchronously from the input hook so the audible response feels instantaneous.

Queue depth: one op at a time. The `ClickAt` op atomically does both cursor-warp and LMouseDown/Up on drain — that's what tab-button activation needs. Every Queue* returns false if an op is already pending; callers log + treat the keypress as consumed.

## Declarations (in source order)

- L44 `uint32_t RaiseIsActiveIfZero(void* control)`: Raise a control's is_active flag from 0 to 1 and return what it was.
- L51 `bool QueueMoveCursor(int x, int y, void* target)`: Queue a cursor-warp to (x, y).
- L58 `bool QueueClickAt(int x, int y, void* target)`: Queue a click-at-point.
- L64 `bool QueueActivate(void* target)`: Queue a direct-activate via vtable[15].HandleInputEvent(0x27, 1).
- L70 `bool QueueEquipSelect(void* panel, void* slot)`: Queue an equip-screen slot activation.
- L76 `bool QueueEquipCommit(void* panel, void* row, void* btn)`: Queue an equip-row commit.
- L89 `bool QueueEquipPickerCancel(void* panel)`: Queue a cancel/back-out of the equip-screen item picker — the equip twin of QueueWorkbenchPickerCancel below, and the reason Escape now reverts instead of silently keeping whatever the last arrow key previewed.
- L98 `bool QueueWorkbenchSlotSelect(void* panel, void* slot)`: Queue a workbench slot-button activation.
- L108 `bool QueueWorkbenchUpgradeCommit(void* panel, void* row, void* btnAssemble)`: Queue a workbench upgrade-panel commit.
- L117 `bool QueueWorkbenchUpgradeInstallK2(void* panel, void* row)`: Queue a KOTOR 2 upgrade install/remove — a single engine call.
- L123 `bool QueueWorkbenchPickerCancel(void* panel)`: Queue a cancel/back-out of the workbench mod-picker.
- L130 `bool QueueSliderInput(void* target, int code)`: Queue a slider value adjustment via vtable[15].HandleInputEvent(code, 1) where code is 500 (increment) or 501 (decrement).
- L146 `bool QueueStoreItemActivate(void* panel, void* row)`: Queue a Store item row trade action.
- L158 `bool QueueCraftRowCommit(void* panel, void* listBox, void* row)`: Queue a K2 workbench / crafting list-row commit.
- L166 `bool QueueGalaxyInput(void* panel, int engineCode, bool announcePlanet)`: Queue a galaxy-map (CSWGuiInGameGalaxyMap) input dispatch.
- L174 `bool QueueWagerInput(void* panel, int code)`: Queue a Pazaak wager-popup less/more step.
- L180 `bool IsPending()`: True if an op is currently queued.
- L185 `void Drain(void* gm)`: Drain the queue.

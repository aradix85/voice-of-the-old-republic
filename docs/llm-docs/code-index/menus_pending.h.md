# menus_pending.h (179 lines)

Source: `patches/Accessibility/menus_pending.h`

The deferred menu-side operation queue: one tagged op at a time. Input handlers call `Queue*` to enqueue; `Drain` runs once per tick from `TickPendingOps` and dispatches the op against deep engine functions (MoveMouseToPosition, manager LMouseDown/Up, vtable[15] HandleInputEvent, OnEnterSlot/OnSelectSlot, OnItemSelected/OnOKPressed).

Why deferred: invoking these mid-input-dispatch recurses through HandleMouseMove → UpdateMouseOverControl, the same re-entrancy class that destabilised earlier listbox-entry hooks. Deferring to the next CSWGuiManager::Update tick (~16ms at 60fps; inaudible) keeps deep engine re-entry off the input-hook stack. Speech still fires synchronously from the input hook so the audible response feels instantaneous.

Queue depth: one op at a time. The `ClickAt` op atomically does both cursor-warp and LMouseDown/Up on drain — that's what tab-button activation needs. Every Queue* returns false if an op is already pending; callers log + treat the keypress as consumed.

## Declarations (in source order)

- L40 `uint32_t RaiseIsActiveIfZero(void* control)`: Raise a control's is_active flag from 0 to 1 and return what it was.
- L44 `bool QueueMoveCursor(int x, int y, void* target)`: Queue a cursor-warp to (x, y).
- L51 `bool QueueClickAt(int x, int y, void* target)`: Queue a click-at-point.
- L57 `bool QueueActivate(void* target)`: Queue a direct-activate via vtable[15].HandleInputEvent(0x27, 1).
- L63 `bool QueueEquipSelect(void* panel, void* slot)`: Queue an equip-screen slot activation.
- L69 `bool QueueEquipCommit(void* panel, void* row, void* btn)`: Queue an equip-row commit.
- L82 `bool QueueEquipPickerCancel(void* panel)`: Queue a cancel/back-out of the equip-screen item picker — the equip twin of QueueWorkbenchPickerCancel below, and the reason Escape now reverts instead of silently keeping whatever the last arrow key previewed.
- L91 `bool QueueWorkbenchSlotSelect(void* panel, void* slot)`: Queue a workbench slot-button activation.
- L101 `bool QueueWorkbenchUpgradeCommit(void* panel, void* row, void* btnAssemble)`: Queue a workbench upgrade-panel commit.
- L110 `bool QueueWorkbenchUpgradeInstallK2(void* panel, void* row)`: Queue a KOTOR 2 upgrade install/remove — a single engine call.
- L116 `bool QueueWorkbenchPickerCancel(void* panel)`: Queue a cancel/back-out of the workbench mod-picker.
- L123 `bool QueueSliderInput(void* target, int code)`: Queue a slider value adjustment via vtable[15].HandleInputEvent(code, 1) where code is 500 (increment) or 501 (decrement).
- L139 `bool QueueStoreItemActivate(void* panel, void* row)`: Queue a Store item row trade action.
- L151 `bool QueueCraftRowCommit(void* panel, void* listBox, void* row)`: Queue a K2 workbench / crafting list-row commit.
- L159 `bool QueueGalaxyInput(void* panel, int engineCode, bool announcePlanet)`: Queue a galaxy-map (CSWGuiInGameGalaxyMap) input dispatch.
- L167 `bool QueueWagerInput(void* panel, int code)`: Queue a Pazaak wager-popup less/more step.
- L171 `bool IsPending()`: True if an op is currently queued.
- L176 `void Drain(void* gm)`: Drain the queue.

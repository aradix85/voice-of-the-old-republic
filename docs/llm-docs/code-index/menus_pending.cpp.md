# menus_pending.cpp (435 lines)

Source: `patches/Accessibility/menus_pending.cpp`

deferred menu-side operation queue.

(why ops are deferred, single-slot queue, ClickAt as combined move+click).

Body of `Drain` mirrors the original `TickPendingOps` block in menus.cpp: same dispatch order (cursor warp → click → activate → equip-select → equip-commit → slider), same engine entry points, same diagnostic log lines. The only behavioural change is at the queue level — every Queue* uniformly debounces against `IsPending()`, where the old code's per-site debounce subsets were inconsistent (e.g. EquipPicker Enter checked EquipSelect/Commit, but Slider Left/Right didn't).

## Declarations (in source order)

- L29 `void Reset()`
- L38 `void* MouseOver(void* gm)`: CSWGuiManager mouseOverControl pointer at +0x8 (per the decompilation in docs/menu-nav-design.md).
- L42 `void DrainMoveCursor(void* gm, const PendingOp& op)`
- L68 `void DrainClickAt(void* gm, const PendingOp& op)`: Combined move + click.
- L93 `void DrainSliderInput(const PendingOp& op)`: Slider value adjustment via vtable[15].HandleInputEvent(500/501, 1).
- L115 `void DrainStoreItemActivate(const PendingOp& op)`: Store item Enter — dispatch the engine's per-mode click handler with the row as param_1.
- L122 `void DrainCraftRowCommit(const PendingOp& op)`: K2 workbench / crafting list-row commit — select the row, then fire the panel's own commit button.
- L130 `void DrainGalaxyInput(const PendingOp& op)`: Galaxy-map planet cycle / accept / cancel.
- L140 `void DrainWagerInput(const PendingOp& op)`: Pazaak wager less/more step — the popup's BTN_LESS / BTN_MORE speed buttons only respond to their push callback, which re-dispatches to the panel's HandleInputEvent (0x2f decrement / 0x30 increment).
- L170 `uint32_t RaiseIsActiveIfZero(void* control)`: Raise a control's is_active flag from 0 to 1 and return what it was.
- L178 `bool QueueMoveCursor(int x, int y, void* target)`
- L187 `bool QueueClickAt(int x, int y, void* target)`
- L196 `bool QueueActivate(void* target)`
- L203 `bool QueueEquipSelect(void* panel, void* slot)`
- L211 `bool QueueEquipCommit(void* panel, void* row, void* btn)`
- L220 `bool QueueEquipPickerCancel(void* panel)`
- L227 `bool QueueWorkbenchSlotSelect(void* panel, void* slot)`
- L235 `bool QueueWorkbenchUpgradeCommit(void* panel, void* row, void* btnAssemble)`
- L244 `bool QueueWorkbenchUpgradeInstallK2(void* panel, void* row)`
- L252 `bool QueueWorkbenchPickerCancel(void* panel)`
- L259 `bool QueueSliderInput(void* target, int code)`
- L267 `bool QueueStoreItemActivate(void* panel, void* row)`
- L275 `bool QueueCraftRowCommit(void* panel, void* listBox, void* row)`
- L284 `bool QueueGalaxyInput(void* panel, int engineCode, bool announcePlanet)`
- L293 `bool QueueWagerInput(void* panel, int code)`
- L301 `bool IsPending()`
- L315 `bool EngineOpsReady(Kind kind)`: Are the engine addresses this op needs actually resolved for the running build?
- L346 `void Drain(void* gm)`

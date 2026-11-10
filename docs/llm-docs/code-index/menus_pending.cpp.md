# menus_pending.cpp (1146 lines)

Source: `patches/Accessibility/menus_pending.cpp`

deferred menu-side operation queue.

(why ops are deferred, single-slot queue, ClickAt as combined move+click).

Body of `Drain` mirrors the original `TickPendingOps` block in menus.cpp: same dispatch order (cursor warp → click → activate → equip-select → equip-commit → slider), same engine entry points, same diagnostic log lines. The only behavioural change is at the queue level — every Queue* uniformly debounces against `IsPending()`, where the old code's per-site debounce subsets were inconsistent (e.g. EquipPicker Enter checked EquipSelect/Commit, but Slider Left/Right didn't).

## Declarations (in source order)

- L68 `uint32_t RaiseIsActiveIfZero(void* control)`: Raise a control's is_active flag from 0 to 1 and return what it was.
- L118 `void Reset()`
- L130 `void* ReadUpgradeSlotInstalled(void* panel)`: K2: the installed-mod pointer (field35) of the currently-open upgrade slot.
- L148 `bool IsUpgradePickerNoneRow(void* row)`: K2: true iff `row` is the engine's "-" none/remove picker entry (key -1).
- L160 `bool QueueMoveCursor(int x, int y, void* target)`
- L169 `bool QueueClickAt(int x, int y, void* target)`
- L178 `bool QueueActivate(void* target)`
- L185 `bool QueueEquipSelect(void* panel, void* slot)`
- L193 `bool QueueEquipCommit(void* panel, void* row, void* btn)`
- L202 `bool QueueEquipPickerCancel(void* panel)`
- L209 `bool QueueWorkbenchSlotSelect(void* panel, void* slot)`
- L217 `bool QueueWorkbenchUpgradeCommit(void* panel, void* row, void* btnAssemble)`
- L226 `bool QueueWorkbenchUpgradeInstallK2(void* panel, void* row)`
- L234 `bool QueueWorkbenchPickerCancel(void* panel)`
- L241 `bool QueueSliderInput(void* target, int code)`
- L249 `bool QueueStoreItemActivate(void* panel, void* row)`
- L257 `bool QueueCraftRowCommit(void* panel, void* listBox, void* row)`
- L266 `bool QueueGalaxyInput(void* panel, int engineCode, bool announcePlanet)`
- L275 `bool QueueWagerInput(void* panel, int code)`
- L283 `bool IsPending()`
- L297 `bool EngineOpsReady(Kind kind)`: Are the engine addresses this op needs actually resolved for the running build?
- L335 `void* MouseOver(void* gm)`: CSWGuiManager mouseOverControl pointer at +0x8 (per the decompilation in docs/menu-nav-design.md).
- L339 `void DrainMoveCursor(void* gm, const PendingOp& op)`
- L365 `void DrainClickAt(void* gm, const PendingOp& op)`: Combined move + click.
- L419 `void DrainActivate(const PendingOp& op)`: Direct activate via vtable[15].HandleInputEvent(0x27, 1).
- L664 `void DrainEquipSelect(const PendingOp& op)`: Equip-screen slot activation.
- L690 `void DrainEquipCommit(const PendingOp& op)`: Equip-row commit.
- L739 `void DrainEquipPickerCancel(const PendingOp& op)`: Equip-picker cancel — the engine's own "back out of the item zone".
- L767 `void DrainWorkbenchSlotSelect(const PendingOp& op)`: Workbench slot activation.
- L892 `void DrainWorkbenchUpgradeCommit(const PendingOp& op)`: Workbench upgrade commit.
- L932 `void DrainWorkbenchUpgradeInstallK2(const PendingOp& op)`: K2 upgrade install/remove — ONE step.
- L983 `void DrainWorkbenchPickerCancel(const PendingOp& op)`: Back out of the mod-picker without committing (Esc).
- L1004 `void DrainSliderInput(const PendingOp& op)`: Slider value adjustment via vtable[15].HandleInputEvent(500/501, 1).
- L1026 `void DrainStoreItemActivate(const PendingOp& op)`: Store item Enter — dispatch the engine's per-mode click handler with the row as param_1.
- L1033 `void DrainCraftRowCommit(const PendingOp& op)`: K2 workbench / crafting list-row commit — select the row, then fire the panel's own commit button.
- L1041 `void DrainGalaxyInput(const PendingOp& op)`: Galaxy-map planet cycle / accept / cancel.
- L1051 `void DrainWagerInput(const PendingOp& op)`: Pazaak wager less/more step — the popup's BTN_LESS / BTN_MORE speed buttons only respond to their push callback, which re-dispatches to the panel's HandleInputEvent (0x2f decrement / 0x30 increment).
- L1057 `void Drain(void* gm)`

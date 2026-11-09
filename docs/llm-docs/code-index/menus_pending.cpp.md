# menus_pending.cpp (1114 lines)

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
- L328 `void Drain(void* gm)`

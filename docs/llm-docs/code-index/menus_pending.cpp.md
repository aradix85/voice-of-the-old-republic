# menus_pending.cpp (336 lines)

Source: `patches/Accessibility/menus_pending.cpp`

The deferred menu-side operation queue (see menus_pending.h for why the ops are deferred). Here: the single-slot queue, the check that an op's engine addresses exist on this build, Drain, and the small ops (cursor, click, slider, store, crafting, galaxy map, wager).

## Declarations (in source order)

- L22 `void Reset()`
- L27 `bool Queue(const PendingOp& op)`: Takes the op if the slot is free; false while another op is pending.
- L33 `PendingOp Op(Kind kind, void* a, void* b = nullptr, void* c = nullptr)`
- L42 `PendingOp CodeOp(Kind kind, void* a, int code)`
- L48 `PendingOp PointOp(Kind kind, int x, int y, void* target)`
- L60 `void* MouseOver(void* gm)`: CSWGuiManager mouseOverControl pointer at +0x8 (per the decompilation in docs/menu-nav-design.md).
- L64 `void DrainMoveCursor(void* gm, const PendingOp& op)`
- L86 `void DrainClickAt(void* gm, const PendingOp& op)`: Combined move + click.
- L111 `void DrainSliderInput(const PendingOp& op)`: Slider value adjustment via HandleInputEvent(500/501, 1).
- L125 `void DrainStoreItemActivate(const PendingOp& op)`: Store item Enter — dispatch the engine's per-mode click handler with the row as param_1.
- L132 `void DrainCraftRowCommit(const PendingOp& op)`: K2 workbench / crafting list-row commit — select the row, then fire the panel's own commit button.
- L140 `void DrainGalaxyInput(const PendingOp& op)`: Galaxy-map planet cycle / accept / cancel.
- L150 `void DrainWagerInput(const PendingOp& op)`: Pazaak wager less/more step — the popup's BTN_LESS / BTN_MORE speed buttons only respond to their push callback, which re-dispatches to the panel's HandleInputEvent (0x2f decrement / 0x30 increment).
- L160 `bool EngineOpsReady(Kind kind)`: Are the engine addresses this op needs resolved for the running build?
- L212 `uint32_t RaiseIsActiveIfZero(void* control)`: Raise a control's is_active flag from 0 to 1 and return what it was.
- L220 `bool QueueMoveCursor(int x, int y, void* target)`
- L224 `bool QueueClickAt(int x, int y, void* target)`
- L228 `bool QueueActivate(void* target)`
- L232 `bool QueueEquipSelect(void* panel, void* slot)`
- L236 `bool QueueEquipCommit(void* panel, void* row, void* btn)`
- L240 `bool QueueEquipPickerCancel(void* panel)`
- L244 `bool QueueWorkbenchSlotSelect(void* panel, void* slot)`
- L248 `bool QueueWorkbenchUpgradeCommit(void* panel, void* row, void* btnAssemble)`
- L252 `bool QueueWorkbenchUpgradeInstallK2(void* panel, void* row)`
- L256 `bool QueueWorkbenchPickerCancel(void* panel)`
- L260 `bool QueueSliderInput(void* target, int code)`
- L264 `bool QueueStoreItemActivate(void* panel, void* row)`
- L268 `bool QueueCraftRowCommit(void* panel, void* listBox, void* row)`
- L272 `bool QueueGalaxyInput(void* panel, int engineCode, bool announcePlanet)`
- L278 `bool QueueWagerInput(void* panel, int code)`
- L282 `bool IsPending()`
- L286 `void Drain(void* gm)`

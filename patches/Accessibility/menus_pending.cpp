// The deferred menu-side operation queue (see menus_pending.h for why the
// ops are deferred). Here: the single-slot queue, the check that an op's
// engine addresses exist on this build, Drain, and the small ops (cursor,
// click, slider, store, crafting, galaxy map, wager).

#include "menus_pending_internal.h"

#include "engine_manager.h"
#include "engine_offsets.h"
#include "log.h"
#include "menus_crafting.h"
#include "menus_galaxymap.h"
#include "menus_store.h"
#include "minigame_pazaak.h"

namespace acc::menus::pending {

namespace {

PendingOp g_op;

void Reset() {
    g_op = PendingOp{};
}

// Takes the op if the slot is free; false while another op is pending.
bool Queue(const PendingOp& op) {
    if (g_op.kind != Kind::None) return false;
    g_op = op;
    return true;
}

PendingOp Op(Kind kind, void* a, void* b = nullptr, void* c = nullptr) {
    PendingOp op;
    op.kind = kind;
    op.a = a;
    op.b = b;
    op.c = c;
    return op;
}

PendingOp CodeOp(Kind kind, void* a, int code) {
    PendingOp op = Op(kind, a);
    op.code = code;
    return op;
}

PendingOp PointOp(Kind kind, int x, int y, void* target) {
    PendingOp op = Op(kind, target);
    op.x = x;
    op.y = y;
    return op;
}

// CSWGuiManager mouseOverControl pointer at +0x8 (per the decompilation
// in docs/menu-nav-design.md). Reading it directly lets us verify what
// the engine's hit-test resolved the cursor to — the difference between
// "click landed on tab T" and "click landed on the inline listbox" is
// invisible from cursor coords alone.
void* MouseOver(void* gm) {
    return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(gm) + 0x8);
}

void DrainMoveCursor(void* gm, const PendingOp& op) {
    auto move = reinterpret_cast<PFN_MoveMouseToPosition>(kAddrMoveMouseToPosition);
    // mouseOver before and after the move tells whether MoveMouseToPosition
    // itself produces a shifted hit-test result or the engine pre-set
    // mouseOver during panel init and the move never refreshed it.
    void* moBefore = MouseOver(gm);
    move(gm, op.x, op.y);
    acclog::Write("Update", "MoveMouseToPosition(%d, %d) target=%p mouseOver before=%p after=%p",
                  op.x, op.y, op.a,
                  moBefore, MouseOver(gm));
}

// Combined move + click. Click-sim via manager's HandleLMouseDown/Up.
// Dispatches against mouseOverControl, which on Options-style tabbed
// panels resolves to the button one step above the chain target
// (a consistent 45-px hit-test shift). That activates the wrong tab.
//
// Direct vtable[6]/[7] on the chain target crashes the game on the second
// tab click: the button's own HandleLMouseDown/Up depend on manager-side
// state (probably the +0x1c mouse_held bit and/or other setup we'd skip).
// The off-by-one needs a different approach; until then this pipeline
// keeps the behaviour stable.
void DrainClickAt(void* gm, const PendingOp& op) {
    auto move = reinterpret_cast<PFN_MoveMouseToPosition>(kAddrMoveMouseToPosition);
    auto down = reinterpret_cast<PFN_ManagerLMouseDown>(kAddrManagerLMouseDown);
    auto up   = reinterpret_cast<PFN_ManagerLMouseUp>(kAddrManagerLMouseUp);
    void* moBefore = MouseOver(gm);
    move(gm, op.x, op.y);
    acclog::Write("Update", "MoveMouseToPosition(%d, %d) target=%p mouseOver before=%p after=%p",
                  op.x, op.y, op.a,
                  moBefore, MouseOver(gm));
    moBefore = MouseOver(gm);
    int dResult = down(gm, /*press=*/1);
    void* moAfterDown = MouseOver(gm);
    int uResult = up(gm);
    void* moAfterUp = MouseOver(gm);
    acclog::Write("Update", "click-sim Down=%d Up=%d at (%d,%d) target=%p "
                  "mouseOver before=%p afterDown=%p afterUp=%p",
                  dResult, uResult, op.x, op.y, op.a,
                  moBefore, moAfterDown, moAfterUp);
}

// Slider value adjustment via HandleInputEvent(500/501, 1). The slider's
// handler runs SetCurValue (clamped to [0, max_value]) and the gui_object
// callback that propagates to the audio system, then plays the click
// feedback sound. The focus monitor catches the cur_value change at +0x74
// on the next tick.
void DrainSliderInput(const PendingOp& op) {
    auto fn = HandleInputEventOf(op.a);
    if (!fn) return;
    acclog::Write("Update", "slider HandleInputEvent target=%p code=%d", op.a, op.code);
    fn(op.a, op.code, 1);
}

// Store item Enter — dispatch the engine's per-mode click handler
// with the row as param_1. The engine reads the row's obj_id at
// +0x1c4 and either opens the confirmation MessageBox or sells/buys
// directly (depending on its internal cost vs. level threshold).
// After the engine call, PopulateInventoryItemListBox /
// PopulateStoreItemListBox repopulates rows; menus_store's per-tick
// monitor detects the listbox size change and rebinds the chain.
void DrainStoreItemActivate(const PendingOp& op) {
    acc::menus::store::DispatchTradeAction(op.a, op.b);
}

// K2 workbench / crafting list-row commit — select the row, then fire
// the panel's own commit button. No direct engine addresses involved
// (button dispatch goes through vtable[15]), so no EngineOpsReady gate.
void DrainCraftRowCommit(const PendingOp& op) {
    acc::menus::crafting::DispatchRowCommit(op.a, op.b, op.c);
}

// Galaxy-map planet cycle / accept / cancel. DispatchInput calls the
// panel's own HandleInputEvent (the engine's NextPlanet/PrevPlanet skip
// hidden / unselectable planets), then re-reads LBL_PLANETNAME when this
// was an Up/Down cycle (op.x == 1).
void DrainGalaxyInput(const PendingOp& op) {
    acc::menus::galaxymap::DispatchInput(op.a, op.code, op.x != 0);
}

// Pazaak wager less/more step — the popup's BTN_LESS / BTN_MORE speed
// buttons only respond to their push callback, which re-dispatches to the
// panel's HandleInputEvent (0x2f decrement / 0x30 increment). The generic
// vtable[15] activate (0x27) they ignore, so this is the dedicated Enter
// path. The engine clamps to [1, max] and repaints LBL_WAGERVAL; the
// per-tick wager service announces the new amount.
void DrainWagerInput(const PendingOp& op) {
    acc::pazaak::DispatchWagerInput(op.a, op.code);
}

// Are the engine addresses this op needs resolved for the running build?
// On KOTOR 1 every one of these is known; on KOTOR 2 an unported one
// resolves to 0, and calling it would fault outside any SEH frame. A
// lookup per op rather than one "is this KOTOR 2" test: the ops resolve
// independently.
bool EngineOpsReady(Kind kind) {
    using acc::addr::Ok;
    switch (kind) {
    case Kind::None:
        return true;
    case Kind::MoveCursor:
        return Ok(kAddrMoveMouseToPosition);
    case Kind::ClickAt:
        return Ok(kAddrMoveMouseToPosition) && Ok(kAddrManagerLMouseDown) &&
               Ok(kAddrManagerLMouseUp);
    case Kind::EquipSelect:
        return Ok(kAddrInGameEquipOnEnterSlot) && Ok(kAddrInGameEquipOnSelectSlot);
    case Kind::EquipCommit:
        return Ok(kAddrInGameEquipOnItemSelected) && Ok(kAddrInGameEquipOnOKPressed);
    case Kind::WorkbenchSlotSelect:
        return Ok(kAddrCSWGuiUpgradeOnEnterSlot) && Ok(kAddrCSWGuiUpgradeOnSlotSelected);
    case Kind::WorkbenchUpgradeCommit:
        return Ok(kAddrCSWGuiUpgradeOnUpgradeSelected) && Ok(kAddrCSWGuiUpgradeOnAssemble);
    case Kind::WorkbenchUpgradeInstallK2:
        return Ok(kAddrCSWGuiUpgradeOnUpgradeSelected);
    case Kind::WorkbenchPickerCancel:
        return Ok(kAddrCSWGuiUpgradeShowItems);
    default:
        // Activate / EquipPickerCancel / SliderInput / StoreItemActivate /
        // CraftRowCommit / GalaxyInput / WagerInput dispatch through the
        // control's (or panel's) own vtable rather than a hardcoded address,
        // so they carry no build dependency.
        return true;
    }
}

}  // namespace

// Raise a control's is_active flag from 0 to 1 and return what it was.
//
// Every engine dispatch we fire by hand needs this first, because several
// engine onClick handlers gate on `this->is_active != 0` and silently divert
// to a different code path when the flag is zero. For MessageBox OK buttons
// that divergent path appeared to corrupt engine state during close — three
// Windows __fastfail dumps all landed between a FireActivate and the next
// frame, with the same MessageBox singleton in the foreground each time.
//
// ONLY raise 0 -> 1. HandleLMouseDown's real behaviour is a CONDITIONAL
// raise, not an unconditional write. Some controls (tab buttons in
// InGameOptions, equip-slot buttons in InGameEquip) carry non-zero
// engine-bookkeeping values in this field — a selected-tab marker, equip-state
// flags. Clobbering those with 1 corrupts the engine's view of which tab is
// selected, which surfaces later as a crash in an unrelated subsystem on the
// next render/physics tick (5 -> 1 on the Feedback tab was followed by a
// crash in CSWRoomSurfaceMesh AABB traversal after a tab-subdialog close).
// One home for the rule and its reason, so an unconditional write is not
// reintroduced at a call site.
uint32_t RaiseIsActiveIfZero(void* control) {
    uint32_t* isActive = reinterpret_cast<uint32_t*>(
        reinterpret_cast<unsigned char*>(control) + kControlIsActiveOffset);
    uint32_t prev = *isActive;
    if (prev == 0) *isActive = 1;
    return prev;
}

bool QueueMoveCursor(int x, int y, void* target) {
    return Queue(PointOp(Kind::MoveCursor, x, y, target));
}

bool QueueClickAt(int x, int y, void* target) {
    return Queue(PointOp(Kind::ClickAt, x, y, target));
}

bool QueueActivate(void* target) {
    return Queue(Op(Kind::Activate, target));
}

bool QueueEquipSelect(void* panel, void* slot) {
    return Queue(Op(Kind::EquipSelect, panel, slot));
}

bool QueueEquipCommit(void* panel, void* row, void* btn) {
    return Queue(Op(Kind::EquipCommit, panel, row, btn));
}

bool QueueEquipPickerCancel(void* panel) {
    return Queue(Op(Kind::EquipPickerCancel, panel));
}

bool QueueWorkbenchSlotSelect(void* panel, void* slot) {
    return Queue(Op(Kind::WorkbenchSlotSelect, panel, slot));
}

bool QueueWorkbenchUpgradeCommit(void* panel, void* row, void* btnAssemble) {
    return Queue(Op(Kind::WorkbenchUpgradeCommit, panel, row, btnAssemble));
}

bool QueueWorkbenchUpgradeInstallK2(void* panel, void* row) {
    return Queue(Op(Kind::WorkbenchUpgradeInstallK2, panel, row));
}

bool QueueWorkbenchPickerCancel(void* panel) {
    return Queue(Op(Kind::WorkbenchPickerCancel, panel));
}

bool QueueSliderInput(void* target, int code) {
    return Queue(CodeOp(Kind::SliderInput, target, code));
}

bool QueueStoreItemActivate(void* panel, void* row) {
    return Queue(Op(Kind::StoreItemActivate, panel, row));
}

bool QueueCraftRowCommit(void* panel, void* listBox, void* row) {
    return Queue(Op(Kind::CraftRowCommit, panel, listBox, row));
}

bool QueueGalaxyInput(void* panel, int engineCode, bool announcePlanet) {
    PendingOp op = CodeOp(Kind::GalaxyInput, panel, engineCode);
    op.x = announcePlanet ? 1 : 0;
    return Queue(op);
}

bool QueueWagerInput(void* panel, int code) {
    return Queue(CodeOp(Kind::WagerInput, panel, code));
}

bool IsPending() {
    return g_op.kind != Kind::None;
}

void Drain(void* gm) {
    if (g_op.kind == Kind::None) return;

    if (!gm) {
        acclog::Write("Update", "pending op but GuiManager singleton is NULL");
        Reset();
        return;
    }

    // Snapshot then clear-then-dispatch. Clearing first means a re-entrant
    // engine callback (which we deliberately decouple from the input hook,
    // but defence-in-depth) sees an empty queue rather than the half-
    // dispatched op.
    PendingOp op = g_op;
    Reset();

    // Every op CALLS the engine, and an address not resolved for the running
    // build is 0: dispatching would call through a null pointer outside any
    // __try. These ops fire on Enter, so that would take the process down on
    // the first activation; declining leaves a missing feature that says so
    // in the log.
    if (!EngineOpsReady(op.kind)) {
        acclog::Write("Update",
                      "pending op kind=%d DECLINED — engine addresses not "
                      "resolved for this build",
                      static_cast<int>(op.kind));
        return;
    }

    switch (op.kind) {
    case Kind::None:                      break;
    case Kind::MoveCursor:                DrainMoveCursor(gm, op);              break;
    case Kind::ClickAt:                   DrainClickAt(gm, op);                 break;
    case Kind::Activate:                  DrainActivate(op);                    break;
    case Kind::EquipSelect:               DrainEquipSelect(op);                 break;
    case Kind::EquipCommit:               DrainEquipCommit(op);                 break;
    case Kind::EquipPickerCancel:         DrainEquipPickerCancel(op);           break;
    case Kind::WorkbenchSlotSelect:       DrainWorkbenchSlotSelect(op);         break;
    case Kind::WorkbenchUpgradeCommit:    DrainWorkbenchUpgradeCommit(op);      break;
    case Kind::WorkbenchUpgradeInstallK2: DrainWorkbenchUpgradeInstallK2(op);   break;
    case Kind::WorkbenchPickerCancel:     DrainWorkbenchPickerCancel(op);       break;
    case Kind::SliderInput:               DrainSliderInput(op);                 break;
    case Kind::StoreItemActivate:         DrainStoreItemActivate(op);           break;
    case Kind::CraftRowCommit:            DrainCraftRowCommit(op);              break;
    case Kind::GalaxyInput:               DrainGalaxyInput(op);                 break;
    case Kind::WagerInput:                DrainWagerInput(op);                  break;
    }
}

}  // namespace acc::menus::pending

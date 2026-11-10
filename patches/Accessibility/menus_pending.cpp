// deferred menu-side operation queue.
//
// (why ops are deferred, single-slot queue, ClickAt as combined move+click).
//
// Body of `Drain` mirrors the original `TickPendingOps` block in menus.cpp:
// same dispatch order (cursor warp → click → activate → equip-select →
// equip-commit → slider), same engine entry points, same diagnostic log
// lines. The only behavioural change is at the queue level — every Queue*
// uniformly debounces against `IsPending()`, where the old code's per-site
// debounce subsets were inconsistent (e.g. EquipPicker Enter checked
// EquipSelect/Commit, but Slider Left/Right didn't).

#include "menus_pending_internal.h"

#include "engine_manager.h"
#include "engine_offsets.h"
#include "log.h"
#include "menus_crafting.h"  // DispatchRowCommit for CraftRowCommit
#include "menus_galaxymap.h"        // DispatchInput for GalaxyInput
#include "minigame_pazaak.h"                 // DispatchWagerInput for WagerInput
#include "menus_store.h"     // DispatchTradeAction for StoreItemActivate

namespace acc::menus::pending {

namespace {

PendingOp g_op;

void Reset() {
    g_op = PendingOp{};
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
    // Capture mouseOver BEFORE the move. Disambiguates whether
    // MoveMouseToPosition itself produces the 45-px-shifted hit-test
    // result (before==something-else, after==shifted-button) vs. the
    // engine pre-setting mouseOver during panel init (before==after,
    // never refreshed by our move at all). See chat.
    void* moBefore = MouseOver(gm);
    move(gm, op.x, op.y);
    acclog::Write("Update", "MoveMouseToPosition(%d, %d) target=%p mouseOver before=%p after=%p",
                  op.x, op.y, op.a,
                  moBefore, MouseOver(gm));
}

// Combined move + click. Click-sim via manager's HandleLMouseDown/Up.
// Dispatches against mouseOverControl, which on Options-style tabbed
// panels resolves to the button one step above the chain target
// (consistent 45-px hit-test shift — see chat investigation). That
// activates the wrong tab.
//
// Direct vtable[6]/[7] on the chain target was tried as a workaround
// (commit reverted) — it crashes the game on the second tab+ click.
// The button's own HandleLMouseDown/Up depend on manager-side state
// (probably the +0x1c mouse_held bit and/or other setup we'd skip).
// Need a different approach for the off-by-1; for now keep the original
// pipeline so behavior is at least stable.
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

// Slider value adjustment via vtable[15].HandleInputEvent(500/501, 1).
// The slider's HandleInputEvent runs SetCurValue (clamped to
// [0, max_value]) and the gui_object callback that propagates to the
// audio system, then plays the click feedback sound. Per-frame focus
// monitor catches the cur_value change at +0x74 on the next tick.
void DrainSliderInput(const PendingOp& op) {
    if (op.a) {
        void** vtable = *reinterpret_cast<void***>(op.a);
        if (vtable) {
            auto fn = reinterpret_cast<PFN_ControlHandleInputEvent>(
                vtable[kVtableHandleInputEvent]);
            if (fn) {
                acclog::Write("Update", "slider HandleInputEvent target=%p code=%d",
                              op.a, op.code);
                fn(op.a, op.code, 1);
            }
        }
    }
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
// per-tick wager observer announces the new amount.
void DrainWagerInput(const PendingOp& op) {
    acc::pazaak::DispatchWagerInput(op.a, op.code);
}

}  // namespace

// Raise a control's is_active flag from 0 to 1 and return what it was.
//
// Every engine dispatch we fire by hand needs this first, because several
// engine onClick handlers gate on `this->is_active != 0` (memory:
// oncontrolentered_is_active_gate) and silently divert to a different code
// path when the flag is zero. For MessageBox OK buttons that divergent path
// appeared to corrupt engine state during close — three Windows __fastfail
// dumps in one day all landed between a FireActivate and the next frame,
// with the same MessageBox singleton in the foreground each time.
//
// ONLY raise 0 -> 1. HandleLMouseDown's real behaviour is a CONDITIONAL
// raise, not an unconditional write. Some controls (tab buttons in
// InGameOptions, equip-slot buttons in InGameEquip) carry non-zero
// engine-bookkeeping values in this field — a selected-tab marker, equip-state
// flags. Clobbering those with 1 corrupts the engine's view of which tab is
// selected, which surfaces later as a crash in an unrelated subsystem on the
// next render/physics tick. A FireActivate audit across 9 events found 5->1
// (Feedback tab) was the only non-trivial clobber in all of testing, and the
// session it appeared in is the one that crashed in CSWRoomSurfaceMesh AABB
// traversal moments after a tab-subdialog close.
//
// This lived inline at seven sites; by the last copy the rationale above had
// been dropped entirely, which is precisely how an unconditional write gets
// reintroduced. One home for the rule and its reason.
uint32_t RaiseIsActiveIfZero(void* control) {
    uint32_t* isActive = reinterpret_cast<uint32_t*>(
        reinterpret_cast<unsigned char*>(control) + kControlIsActiveOffset);
    uint32_t prev = *isActive;
    if (prev == 0) *isActive = 1;
    return prev;
}

bool QueueMoveCursor(int x, int y, void* target) {
    if (g_op.kind != Kind::None) return false;
    g_op.kind = Kind::MoveCursor;
    g_op.x = x;
    g_op.y = y;
    g_op.a = target;
    return true;
}

bool QueueClickAt(int x, int y, void* target) {
    if (g_op.kind != Kind::None) return false;
    g_op.kind = Kind::ClickAt;
    g_op.x = x;
    g_op.y = y;
    g_op.a = target;
    return true;
}

bool QueueActivate(void* target) {
    if (g_op.kind != Kind::None) return false;
    g_op.kind = Kind::Activate;
    g_op.a = target;
    return true;
}

bool QueueEquipSelect(void* panel, void* slot) {
    if (g_op.kind != Kind::None) return false;
    g_op.kind = Kind::EquipSelect;
    g_op.a = panel;
    g_op.b = slot;
    return true;
}

bool QueueEquipCommit(void* panel, void* row, void* btn) {
    if (g_op.kind != Kind::None) return false;
    g_op.kind = Kind::EquipCommit;
    g_op.a = panel;
    g_op.b = row;
    g_op.c = btn;
    return true;
}

bool QueueEquipPickerCancel(void* panel) {
    if (g_op.kind != Kind::None) return false;
    g_op.kind = Kind::EquipPickerCancel;
    g_op.a = panel;
    return true;
}

bool QueueWorkbenchSlotSelect(void* panel, void* slot) {
    if (g_op.kind != Kind::None) return false;
    g_op.kind = Kind::WorkbenchSlotSelect;
    g_op.a = panel;
    g_op.b = slot;
    return true;
}

bool QueueWorkbenchUpgradeCommit(void* panel, void* row, void* btnAssemble) {
    if (g_op.kind != Kind::None) return false;
    g_op.kind = Kind::WorkbenchUpgradeCommit;
    g_op.a = panel;
    g_op.b = row;
    g_op.c = btnAssemble;
    return true;
}

bool QueueWorkbenchUpgradeInstallK2(void* panel, void* row) {
    if (g_op.kind != Kind::None) return false;
    g_op.kind = Kind::WorkbenchUpgradeInstallK2;
    g_op.a = panel;
    g_op.b = row;
    return true;
}

bool QueueWorkbenchPickerCancel(void* panel) {
    if (g_op.kind != Kind::None) return false;
    g_op.kind = Kind::WorkbenchPickerCancel;
    g_op.a = panel;
    return true;
}

bool QueueSliderInput(void* target, int code) {
    if (g_op.kind != Kind::None) return false;
    g_op.kind = Kind::SliderInput;
    g_op.a = target;
    g_op.code = code;
    return true;
}

bool QueueStoreItemActivate(void* panel, void* row) {
    if (g_op.kind != Kind::None) return false;
    g_op.kind = Kind::StoreItemActivate;
    g_op.a = panel;
    g_op.b = row;
    return true;
}

bool QueueCraftRowCommit(void* panel, void* listBox, void* row) {
    if (g_op.kind != Kind::None) return false;
    g_op.kind = Kind::CraftRowCommit;
    g_op.a = panel;
    g_op.b = listBox;
    g_op.c = row;
    return true;
}

bool QueueGalaxyInput(void* panel, int engineCode, bool announcePlanet) {
    if (g_op.kind != Kind::None) return false;
    g_op.kind = Kind::GalaxyInput;
    g_op.a = panel;
    g_op.code = engineCode;
    g_op.x = announcePlanet ? 1 : 0;
    return true;
}

bool QueueWagerInput(void* panel, int code) {
    if (g_op.kind != Kind::None) return false;
    g_op.kind = Kind::WagerInput;
    g_op.a = panel;
    g_op.code = code;
    return true;
}

bool IsPending() {
    return g_op.kind != Kind::None;
}

// Are the engine addresses this op needs actually resolved for the running
// build? On KOTOR 1 every one of these is known and this is always true; on
// KOTOR 2 the unported ones resolve to 0 and calling them would fault outside
// any SEH frame.
//
// Deliberately a lookup rather than a blanket "is this KOTOR 2" test: the ops
// resolve independently, so as the port progresses each becomes available on
// its own without this needing to be revisited as a whole. It also covers the
// KOTOR 1 relink, where an address missing from the rebase table resolves to 0
// for exactly the same reason.
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
        // GalaxyInput / WagerInput dispatch through the control's (or panel's)
        // own vtable rather than a hardcoded address, so they carry no build
        // dependency.
        return true;
    }
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

    // Every branch below CALLS the engine directly, and an engine address that
    // is not resolved for the running build hands back 0 — so dispatching would
    // be a call through a null pointer, outside any __try. That is a crash, not
    // a degraded feature.
    //
    // This is the whole reason the queue exists: these ops fire on activation
    // (Enter on a menu item), so on KOTOR 2 the very first Enter would take the
    // process down while everything else appeared to work. Declining the op
    // instead means activation does nothing and says so in the log, which is a
    // missing feature and diagnosable.
    //
    // Checked per op rather than once, because the ops need different
    // addresses and they resolve independently — see EngineOpsReady below.
    if (op.kind != Kind::None && !EngineOpsReady(op.kind)) {
        acclog::Write("Update",
                      "pending op kind=%d DECLINED — engine addresses not "
                      "resolved for this build (KOTOR 2 port in progress)",
                      static_cast<int>(op.kind));
        return;
    }

    switch (op.kind) {
    case Kind::None:
        break;
    case Kind::MoveCursor:
        DrainMoveCursor(gm, op);
        break;
    case Kind::ClickAt:
        DrainClickAt(gm, op);
        break;
    case Kind::Activate:
        DrainActivate(op);
        break;
    case Kind::EquipSelect:
        DrainEquipSelect(op);
        break;
    case Kind::EquipCommit:
        DrainEquipCommit(op);
        break;
    case Kind::EquipPickerCancel:
        DrainEquipPickerCancel(op);
        break;
    case Kind::WorkbenchSlotSelect:
        DrainWorkbenchSlotSelect(op);
        break;
    case Kind::WorkbenchUpgradeCommit:
        DrainWorkbenchUpgradeCommit(op);
        break;
    case Kind::WorkbenchUpgradeInstallK2:
        DrainWorkbenchUpgradeInstallK2(op);
        break;
    case Kind::WorkbenchPickerCancel:
        DrainWorkbenchPickerCancel(op);
        break;
    case Kind::SliderInput:
        DrainSliderInput(op);
        break;
    case Kind::StoreItemActivate:
        DrainStoreItemActivate(op);
        break;
    case Kind::CraftRowCommit:
        DrainCraftRowCommit(op);
        break;
    case Kind::GalaxyInput:
        DrainGalaxyInput(op);
        break;
    case Kind::WagerInput:
        DrainWagerInput(op);
        break;
    }
}

}  // namespace acc::menus::pending

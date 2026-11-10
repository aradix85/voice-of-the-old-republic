// Internal to the menus_pending module (menus_pending*.cpp); no other file includes
// it. The public interface is menus_pending.h. Here: what the files share.

#pragma once

#include "menus_pending.h"

#include <windows.h>
#include <cstdint>
#include <cstdio>


namespace acc::menus::pending {

enum class Kind {
    None,
    MoveCursor,        // x, y, a = target (self-dedup)
    ClickAt,           // x, y, a = target (self-dedup)
    Activate,          // a = target
    EquipSelect,       // a = panel, b = slot
    EquipCommit,       // a = panel, b = row, c = btn
    EquipPickerCancel, // a = panel — the engine's own cancel (revert + close)
    WorkbenchSlotSelect,    // a = panel, b = slot
    WorkbenchUpgradeCommit, // a = panel, b = row, c = btnAssemble
    WorkbenchUpgradeInstallK2, // a = panel, b = row — K2 one-step install/remove
    WorkbenchPickerCancel,  // a = panel — close the mod-picker zone (re-enable slots)
    SliderInput,       // a = target, code = direction (500 inc / 501 dec)
    StoreItemActivate, // a = panel (CSWGuiStore), b = row (StoreItemEntry)
    CraftRowCommit,    // a = panel (K2 workbench screen), b = listbox,
                       // c = row (commit button re-resolved at drain)
    GalaxyInput,       // a = panel (galaxy map), code = engine event,
                       // x = announce-planet flag (0/1)
    WagerInput,        // a = panel (pazaak wager popup), code = engine event
                       // (0x2f less / 0x30 more)
};

struct PendingOp {
    Kind  kind = Kind::None;
    int   x = 0;
    int   y = 0;
    void* a = nullptr;
    void* b = nullptr;
    void* c = nullptr;
    int   code = 0;
};

// Kept local to the drain block (not a public surface). Needed for the
// slider HandleInputEvent dispatch; menus.cpp has its own copy in its
// FireActivate. Typedef + constant are non-ODR-violating across TUs.
constexpr int kVtableHandleInputEvent = 15;
typedef void (__thiscall* PFN_ControlHandleInputEvent)(void* this_, int code, int state);

// Helpers shared across the files, by the file that defines them.

// menus_pending_activate.cpp
void DrainActivate(const PendingOp& op);

// menus_pending_equip.cpp
void DrainEquipSelect(const PendingOp& op);
void DrainEquipCommit(const PendingOp& op);
void DrainEquipPickerCancel(const PendingOp& op);

// menus_pending_workbench.cpp
void DrainWorkbenchSlotSelect(const PendingOp& op);
void DrainWorkbenchUpgradeCommit(const PendingOp& op);
void DrainWorkbenchUpgradeInstallK2(const PendingOp& op);
void DrainWorkbenchPickerCancel(const PendingOp& op);

}  // namespace acc::menus::pending

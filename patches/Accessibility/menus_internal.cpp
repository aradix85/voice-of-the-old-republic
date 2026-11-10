// Implementations of the acc::menus::detail:: seam declared in
// menus_internal.h.
//
// These helpers were defined in menus.cpp until the Phase-1 structure pass
// (refactoring candidate 1). menus_internal.h has always declared them as
// the "internal-but-exposed-across-TUs" contract shared by menus.cpp,
// menus_extract.cpp, menus_listbox.cpp and menus_keymap.cpp; this file is
// the matching .cpp every other seam header in this directory already has
// (menus_chain, menus_pending, menus_listbox, menus_editbox, menus_monitors).
//
// Nothing here changed behaviourally in the move: the definitions are
// verbatim, including the saveload.gui control IDs and the chargen
// class-label cache, both of which are used only by functions in this file.

#include "menus_internal_internal.h"

#include "log.h"
#include "menus_pending.h"   // QueueButtonByIdActivate defers the activate
#include "engine_game.h"      // IsKotor2 — the save/load control ids differ
#include "engine_offsets.h"
#include "engine_reads.h"

namespace acc::menus::detail {

namespace {

// KOTOR 2's GUI authoring width. Every _p.gui in its gui.bif declares
// MAIN_PNL as 800x600 and the engine stretches that to the window.
constexpr int kKotor2GuiAuthoringWidth = 800;

}  // namespace

// See menus_internal.h for why this exists and when it is a no-op.
int ScaleGuiThresholdPx(int guiPx) {
    if (!acc::game::IsKotor2()) return guiPx;
    HWND hwnd = GetActiveWindow();
    if (!hwnd) return guiPx;
    RECT rc = {0, 0, 0, 0};
    if (!GetClientRect(hwnd, &rc)) return guiPx;
    int clientW = rc.right - rc.left;
    if (clientW <= kKotor2GuiAuthoringWidth) return guiPx;
    return (int)((long long)guiPx * clientW / kKotor2GuiAuthoringWidth);
}

// Center pixel of a control's hit area. Returns false on null control,
// unreadable control, or degenerate extent (zero/negative width/height —
// sometimes seen on hidden panels and templated control prototypes).
//
// SEH-guarded because a non-null pointer is NOT proof the control is alive.
// Cycling sub-screens quickly on KOTOR 2 crashed the process here
// (patch-20260801-232432.log + CrashDumps/swkotor2.exe.46564.dmp: fault
// reading [esi+0xc] — this function's `ext[2]` — at accessibility.dll+0x18FFFA,
// with a freed-and-reused control pointer). The panel had been torn down by
// the screen switch while the chain still held its control; the null check
// passed and the read faulted. Same failure class as the FocusProbe and
// TryPartyPortrait crashes: engine reads here are guarded by convention, and
// this one was the exception.
bool GetControlCenter(void* control, int& outCx, int& outCy) {
    if (!control) return false;
    __try {
        auto* ext = reinterpret_cast<int*>(
            reinterpret_cast<unsigned char*>(control) + kControlExtentOffset);
        int width  = ext[2];
        int height = ext[3];
        if (width <= 0 || height <= 0) return false;
        outCx = ext[0] + width  / 2;
        outCy = ext[1] + height / 2;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Screen-absolute center of a CSWGuiListBox row. Listbox children's extents
// are listbox-local (origin at the listbox's top-left, not the screen) so
// click-sim at row.extent alone lands on dead space. Add the listbox's own
// extent origin to translate. Listboxes themselves are panel-direct children
// whose extents are already screen-absolute (panels render at fixed
// positions), so one accumulation step is sufficient for the InGameEquip
// LB_ITEMS case. If we ever need to click rows in a deeper-nested listbox,
// generalise this into a parent-chain walk.
// True if the control is button-like (CSWGuiButton or its subclasses
// CharButton / ActivatedButton / ButtonToggle) OR a CSWGuiSlider.
// MoveMouseToPosition's hover→active promotion path is safe for buttons but
// crashes when the active control is a label (verified: navigating onto the
// main-menu "New content available…" label froze the game). Sliders are
// included because Sound's Music/Voice/SFX/Movie controls are real sliders
// and we want chain navigation to land on them so we can announce their
// numeric value.
//
// Long-term: replace with a proper CSWGuiControl::GetIsSelectable call
// (vtable lookup at 0x4189d0) to also include editbox / listbox / etc.
bool IsChainNavigable(void* control) {
    if (!control) return false;
    if (CallDowncast(control, kVtableAsButton)        != nullptr) return true;
    if (CallDowncast(control, kVtableAsButtonToggle)  != nullptr) return true;
    if (IsSlider(control))                                        return true;
    return false;
}

// Locate a child control on `panel` by its +0x50 ID field. The .gui-time IDs
// are stable per panel kind, so this is the canonical way to address a known
// control in a known panel without text-matching (which breaks across
// localizations) or relying on panel.controls index (which can shift).
//
// Step 4 of the refactor lifted the listbox-driven panel handlers
// (Container / SaveLoad / EquipPicker) into menus_listbox.cpp; this helper
// now spans both TUs via menus_internal.h.
void* FindControlById(void* panel, int id) {
    if (!panel) return nullptr;
    // SEH: the panel may be freed, and the controls array can carry a
    // non-null garbage tail entry (the chargen TryPartyPortrait crash).
    __try {
        auto* list = reinterpret_cast<CExoArrayList*>(
            reinterpret_cast<unsigned char*>(panel) + kPanelControlsOffset);
        if (!list->data || list->size <= 0) return nullptr;
        int n = list->size > 64 ? 64 : list->size;
        for (int i = 0; i < n; ++i) {
            void* c = list->data[i];
            if (!c) continue;
            int cid = *reinterpret_cast<int*>(
                reinterpret_cast<unsigned char*>(c) + kControlIdOffset);
            if (cid == id) return c;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    return nullptr;
}

// Detect the CSWGuiSaveLoad panel (the "Load Game" / "Save Game"
// dialog). The panel is allocated dynamically when the user activates the
// load/save action, has no slot in CGuiInGame, and so doesn't show up via
// the slot table — but it carries its own vtable, and for a dynamically
// allocated panel the vtable IS its identity: written by the ctor, per-exe
// constant, immune to anything a content mod can put in a .gui file.
//
// This replaces the old structural probe (the {listbox id 0, buttons
// 11/12/14} quartet plus per-control vtable tightening) from before the
// gui-id audit — that shape trusted .gui-authored ids, needed a
// belt-and-braces collision defence against upgrade.gui's identical id
// quartet, and would misidentify on any install with a renumbered
// saveload.gui. HasVtable is SEH-guarded, which this call path needs
// anyway: the save-popup teardown can leave a freed SaveLoad panel
// pointer in the chain for a tick (see menus_chain.cpp).
bool IsSaveLoadPanel(void* panel) {
    return acc::engine::HasVtable(panel, kVtableCSWGuiSaveLoad);
}

// Read the user-visible text of a CExoString-style field on a control. Returns
// nullptr if the field is empty or the c_string pointer is null. The two
// fields we care about on CSWGuiSaveLoadEntry (areaname, lastmodule) are plain
// CExoStrings populated from the save GFF — no TLK indirection, no engine
// rendering callback needed. Output is borrowed from the engine; valid until
// the entry is freed (we use it inline within a single input event).
// SEH-guarded because the caller cannot supply the frame: it uses the returned
// pointer in a snprintf, so the read has to have already succeeded or yielded
// null. The KOTOR 1 offsets are known-good, but kSaveLoadEntry*Offset is still
// Todo for KOTOR 2, where it resolves to acc::off::kUnportedOffset — this would
// then dereference far outside the mapping and take the process down instead of
// falling back to the plain row text. Found by tools/re-scripts/
// handler_chain_audit.py while clearing the KOTOR 2 handler gates.
const char* ReadSaveLoadEntryString(void* entry, size_t fieldOffset) {
    if (!entry) return nullptr;
    __try {
        auto* base = reinterpret_cast<unsigned char*>(entry);
        auto* str  = reinterpret_cast<CExoString*>(base + fieldOffset);
        if (!str || !str->c_string || str->length == 0) return nullptr;
        return str->c_string;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

// Queue activation of a control the caller resolved through an engine-member
// resolver. Mirrors the activate path used by chain-Enter elsewhere: queues
// an Activate op via menus_pending and sets the speech-suppress budget so the
// post-activation focus echo doesn't double-speak. The actual
// vtable[15].HandleInputEvent runs one tick later in TickPendingOps.
//
// Returns false on debounce (any op already pending) or on a null target
// (an unported offset, or a panel that has no such control); caller still
// consumes the keypress in those cases so the engine's stale activeControl
// can't take over.
//
// `logPrefix` is used in the diagnostic log line — pass something like
// "Container: Enter -> BTN_OK".
bool QueueControlActivate(void* target,
                                              const char* logPrefix)
{
    if (acc::menus::pending::IsPending()) {
        acclog::Write(logPrefix, "-- op already pending; ignoring");
        return false;
    }
    if (!target) {
        acclog::Write(logPrefix, "-- target not resolved");
        return false;
    }
    acc::menus::pending::QueueActivate(target);
    acclog::Write(logPrefix, "target=%p", target);
    return true;
}

}  // namespace acc::menus::detail

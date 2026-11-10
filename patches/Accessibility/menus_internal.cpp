// Small control helpers the menus files share: GUI threshold scaling, a
// control's centre and its lookup by id, chain navigability, the save/load
// panel test and entry read, and queued activation.

#include <windows.h>

#include "menus_internal.h"
#include "log.h"
#include "menus_pending.h"
#include "engine_game.h"
#include "engine_offsets.h"
#include "engine_panels.h"
#include "engine_reads.h"

using namespace acc::engine;

namespace acc::menus::detail {

namespace {

// KOTOR 2's GUI authoring width. Every _p.gui in its gui.bif declares
// MAIN_PNL as 800x600 and the engine stretches that to the window.
constexpr int kKotor2GuiAuthoringWidth = 800;

}  // namespace

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

// A degenerate extent (zero or negative width or height) shows up on hidden
// panels and templated control prototypes. SEH-guarded because a non-null
// control can already be freed: a fast sub-screen switch tears the panel
// down while the chain still holds its control.
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

// A button (CSWGuiButton or its subclasses CharButton / ActivatedButton /
// ButtonToggle) or a slider. MoveMouseToPosition's hover-to-active promotion
// is safe for buttons but freezes the game when the active control is a
// label. Sliders are included so the Sound screen's volume sliders can be
// reached and announced with their value.
bool IsChainNavigable(void* control) {
    if (!control) return false;
    if (CallDowncast(control, kVtableAsButton)        != nullptr) return true;
    if (CallDowncast(control, kVtableAsButtonToggle)  != nullptr) return true;
    if (IsSlider(control))                                        return true;
    return false;
}

// The .gui ids are stable per panel kind, unlike the control's text (which
// changes with the language) or its index in panel.controls.
void* FindControlById(void* panel, int id) {
    if (!panel) return nullptr;
    // SEH: the panel may be freed, and the controls array can carry a
    // non-null garbage tail entry.
    __try {
        auto* list = reinterpret_cast<CExoArrayList*>(
            reinterpret_cast<unsigned char*>(panel) + kPanelControlsOffset);
        if (!list->data || list->size <= 0) return nullptr;
        int n = list->size > 64 ? 64 : list->size;
        for (int i = 0; i < n; ++i) {
            void* c = list->data[i];
            if (!c) continue;
            if (ReadControlId(c) == id) return c;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    return nullptr;
}

// The panel is allocated when the user picks Load or Save and has no slot in
// CGuiInGame, so its ctor-written vtable is its identity. HasVtable is
// SEH-guarded, which this path needs: the save-popup teardown can leave a
// freed SaveLoad panel in the chain for a tick.
bool IsSaveLoadPanel(void* panel) {
    return acc::engine::HasVtable(panel, kVtableCSWGuiSaveLoad);
}

// The save-row fields (areaname, lastmodule) are plain CExoStrings from the
// save GFF, valid until the entry is freed. SEH-guarded because
// kSaveLoadEntry*Offset is still unported on KOTOR 2, where the read must
// fall back to null instead of taking the process down.
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

// The activate itself runs one tick later in TickPendingOps.
bool QueueControlActivate(void* target, const char* logPrefix) {
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

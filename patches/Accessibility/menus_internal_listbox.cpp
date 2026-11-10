// Driving a listbox's selection from the keyboard, by our own selection
// write or by the engine's SetSelectedControl.

#include <windows.h>

#include "menus_internal.h"
#include "engine_offsets.h"
#include "engine_panels.h"

using namespace acc::engine;

namespace acc::menus::detail {

namespace {

// The row a step lands on, clamped without wrapping. A selection below
// minSel (typically the engine's initial -1) lands on minSel whatever the
// direction.
short TargetRow(ListBoxNavOp op, short oldSel, short minSel, int rowCount) {
    const short last = (short)(rowCount - 1);
    if (op == ListBoxNavOp::JumpFirst) return minSel;
    if (op == ListBoxNavOp::JumpLast) return last < minSel ? minSel : last;
    if (oldSel < minSel) return minSel;
    if (op == ListBoxNavOp::StepDown) {
        return oldSel + 1 > last ? last : (short)(oldSel + 1);
    }
    return oldSel - 1 < minSel ? minSel : (short)(oldSel - 1);
}

// The step itself, outside the SEH frame so that frame holds nothing that
// needs unwinding (MSVC C2712).
bool DriveBody(void* listbox, ListBoxNavOp op, short minSel, bool viaEngine,
               ListBoxNavResult& out) {
    auto* base = reinterpret_cast<unsigned char*>(listbox);
    auto* rows = reinterpret_cast<CExoArrayList*>(base + kListBoxControlsOffset);
    int rowCount = rows->data ? rows->size : 0;
    if (rowCount <= 0) return false;

    auto* sel = reinterpret_cast<short*>(base + kListBoxSelectionIndexOffset);
    short oldSel = *sel;
    short newSel = TargetRow(op, oldSel, minSel, rowCount);

    if (viaEngine) {
        // Re-assert even on a boundary clamp so a frame of hover drift is
        // corrected on the next keypress; the select sound plays only when
        // the selection moves. SetSelectedControl may clamp on its own, so
        // its result is read back.
        auto setSel = reinterpret_cast<PFN_CSWGuiListBoxSetSelectedControl>(
            kAddrCSWGuiListBoxSetSelectedControl);
        setSel(listbox, newSel, newSel != oldSel ? 1 : 0);
        newSel = *sel;
    } else if (newSel != oldSel) {
        *sel = newSel;
        auto* top = reinterpret_cast<short*>(base + kListBoxTopVisibleIndexOffset);
        short perPage = *reinterpret_cast<short*>(base + kListBoxItemsPerPageOffset);
        if (perPage <= 0) perPage = 1;
        if (newSel < *top) {
            *top = newSel;
        } else if (newSel >= *top + perPage) {
            *top = (short)(newSel - perPage + 1);
        }
    }

    out.oldSel   = oldSel;
    out.newSel   = newSel;
    out.rowCount = rowCount;
    out.row      = (newSel >= 0 && newSel < rowCount) ? rows->data[newSel]
                                                       : nullptr;
    return true;
}

// A stale or unported listbox pointer declines the step instead of taking
// the process down; the engine path would otherwise also hand a wild `this`
// to SetSelectedControl.
bool DriveGuarded(void* listbox, ListBoxNavOp op, short minSel, bool viaEngine,
                  ListBoxNavResult& out) {
    out = {};
    if (!listbox) return false;
    __try {
        return DriveBody(listbox, op, minSel, viaEngine, out);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        out = {};
        return false;
    }
}

}  // namespace

bool DriveListBoxSelection(void* listbox, ListBoxNavOp op, short minSel,
                           ListBoxNavResult& out) {
    return DriveGuarded(listbox, op, minSel, /*viaEngine=*/false, out);
}

bool DriveListBoxSelectionEngine(void* listbox, ListBoxNavOp op, short minSel,
                                 ListBoxNavResult& out) {
    return DriveGuarded(listbox, op, minSel, /*viaEngine=*/true, out);
}

}  // namespace acc::menus::detail

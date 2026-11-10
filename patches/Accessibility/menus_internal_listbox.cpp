// Driving a listbox's selection from the keyboard: our own selection write, and the engine's own
// step for the screens that need it.

#include "menus_internal_internal.h"

#include "engine_offsets.h"

namespace acc::menus::detail {

// ListBoxNavResult struct + DriveListBoxSelection signature now live in
// menus_internal.h (Step 4 — listbox-driven panels lifted to
// menus_listbox.cpp). The function is still defined here because the
// dialog/container monitors call it too.
//
// `minSel` is the lowest selectable row index. Pass 0 for normal listboxes;
// pass 1 for the equip-picker LB_ITEMS where row 0 is the .gui-time
// PROTOITEM template (verified empirically — see
// docs/equip-flow-investigation.md). Existing selection_index < minSel
// (typically the engine's initial -1) lands on `minSel` regardless of
// direction, closer to user expectation than wrapping or staying silent.
//
// Returns false iff `listbox` is null or has rowCount==0; caller logs +
// ignores. On true, `out` is fully populated.
static bool DriveListBoxSelectionBody(void* listbox,
                                      acc::menus::detail::ListBoxNavOp op,
                                      short minSel,
                                      acc::menus::detail::ListBoxNavResult& out);

static bool DriveListBoxSelectionEngineBody(
    void* listbox, acc::menus::detail::ListBoxNavOp op, short minSel,
    acc::menus::detail::ListBoxNavResult& out);

bool DriveListBoxSelection(void* listbox, ListBoxNavOp op,
                                               short minSel,
                                               ListBoxNavResult& out)
{
    out = {};
    if (!listbox) return false;
    // Every field access below is an engine read and belongs under SEH like
    // the rest of this codebase's engine reads. The guard is what turns a
    // stale or unported-offset listbox pointer into a declined navigation
    // instead of a process kill — the KOTOR 2 in-world arrow-key crash
    // (2026-08-01) died on the very first read here. Callers already treat
    // false as "nothing to navigate".
    __try {
        return DriveListBoxSelectionBody(listbox, op, minSel, out);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        out = {};
        return false;
    }
}

// The body, split out so the SEH wrapper above stays free of anything needing
// unwinding (MSVC C2712) — see the CLAUDE.md note on __try and objects.
static bool DriveListBoxSelectionBody(void* listbox,
                                      acc::menus::detail::ListBoxNavOp op,
                                      short minSel,
                                      acc::menus::detail::ListBoxNavResult& out)
{
    using acc::menus::detail::ListBoxNavOp;
    auto* lbBase = reinterpret_cast<unsigned char*>(listbox);
    auto* lbList = reinterpret_cast<CExoArrayList*>(
        lbBase + kListBoxControlsOffset);
    int rowCount = (lbList && lbList->data) ? lbList->size : 0;
    if (rowCount <= 0) {
        out.rowCount = 0;
        return false;
    }

    short* selPtr = reinterpret_cast<short*>(
        lbBase + kListBoxSelectionIndexOffset);
    short* topPtr = reinterpret_cast<short*>(
        lbBase + kListBoxTopVisibleIndexOffset);
    short* ippPtr = reinterpret_cast<short*>(
        lbBase + kListBoxItemsPerPageOffset);

    short oldSel = *selPtr;
    short newSel;
    if (op == ListBoxNavOp::JumpFirst) {
        newSel = minSel;
    } else if (op == ListBoxNavOp::JumpLast) {
        newSel = (short)(rowCount - 1);
        if (newSel < minSel) newSel = minSel;
    } else if (oldSel < minSel) {
        // Pre-StepUp/StepDown anchoring: any stale -1 / out-of-range
        // selection lands on minSel regardless of direction (matches
        // user expectation better than a wrap or silent no-op).
        newSel = minSel;
    } else if (op == ListBoxNavOp::StepDown) {
        newSel = (short)(oldSel + 1);
        if (newSel >= rowCount) newSel = (short)(rowCount - 1);
    } else {  // StepUp
        newSel = (short)(oldSel - 1);
        if (newSel < minSel) newSel = minSel;
    }

    if (newSel != oldSel) {
        *selPtr = newSel;
        short ipp = *ippPtr;
        short top = *topPtr;
        if (ipp <= 0) ipp = 1;
        if (newSel < top) {
            *topPtr = newSel;
        } else if (newSel >= top + ipp) {
            *topPtr = (short)(newSel - ipp + 1);
        }
    }

    out.oldSel   = oldSel;
    out.newSel   = newSel;
    out.rowCount = rowCount;
    out.row      = (newSel >= 0 && newSel < rowCount) ? lbList->data[newSel]
                                                       : nullptr;
    return true;
}

bool DriveListBoxSelectionEngine(void* listbox,
                                                     ListBoxNavOp op,
                                                     short minSel,
                                                     ListBoxNavResult& out)
{
    out = {};
    if (!listbox) return false;
    // SEH for the same reason as its raw-write twin above — this one also
    // CALLS the engine's SetSelectedControl on the pointer, so a bad listbox
    // would hand a wild `this` to engine code.
    __try {
        return DriveListBoxSelectionEngineBody(listbox, op, minSel, out);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        out = {};
        return false;
    }
}

static bool DriveListBoxSelectionEngineBody(
    void* listbox, acc::menus::detail::ListBoxNavOp op, short minSel,
    acc::menus::detail::ListBoxNavResult& out)
{
    using acc::menus::detail::ListBoxNavOp;
    auto* lbBase = reinterpret_cast<unsigned char*>(listbox);
    auto* lbList = reinterpret_cast<CExoArrayList*>(
        lbBase + kListBoxControlsOffset);
    int rowCount = (lbList && lbList->data) ? lbList->size : 0;
    if (rowCount <= 0) {
        out.rowCount = 0;
        return false;
    }

    short oldSel = *reinterpret_cast<short*>(
        lbBase + kListBoxSelectionIndexOffset);

    // Identical no-wrap clamp to DriveListBoxSelection — only the commit path
    // differs (engine SetSelectedControl vs. raw field write).
    short newSel;
    if (op == ListBoxNavOp::JumpFirst) {
        newSel = minSel;
    } else if (op == ListBoxNavOp::JumpLast) {
        newSel = (short)(rowCount - 1);
        if (newSel < minSel) newSel = minSel;
    } else if (oldSel < minSel) {
        newSel = minSel;
    } else if (op == ListBoxNavOp::StepDown) {
        newSel = (short)(oldSel + 1);
        if (newSel >= rowCount) newSel = (short)(rowCount - 1);
    } else {  // StepUp
        newSel = (short)(oldSel - 1);
        if (newSel < minSel) newSel = minSel;
    }

    // Drive the engine's real selection. Re-assert even on a boundary clamp
    // (newSel == oldSel) so a frame of hover drift is corrected on the next
    // keypress; only play the select sound when the selection actually moves.
    auto setSel = reinterpret_cast<PFN_CSWGuiListBoxSetSelectedControl>(
        kAddrCSWGuiListBoxSetSelectedControl);
    setSel(listbox, newSel, newSel != oldSel ? 1 : 0);

    // Read selection_index back — SetSelectedControl is authoritative (it can
    // clamp internally), so the announce/commit see exactly what the engine set.
    short engineSel = *reinterpret_cast<short*>(
        lbBase + kListBoxSelectionIndexOffset);
    out.oldSel   = oldSel;
    out.newSel   = engineSel;
    out.rowCount = rowCount;
    out.row      = (engineSel >= 0 && engineSel < rowCount)
                       ? lbList->data[engineSel]
                       : nullptr;
    return true;
}

}  // namespace acc::menus::detail

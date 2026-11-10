// The dialogue reply monitor: speaking the selected reply with its position, on navigation and when the engine swaps in the next node's replies.

#include "menus_monitors_internal.h"

#include "engine_offsets.h"
#include "engine_reads.h"
#include "menus_extract.h"
#include "strings.h"
#include "speech.h"
#include "tutorial_popup.h"  // synthetic Trask-line tutorial popup (Surface 2)

namespace acc::menus::monitors {

namespace {

// ============================================================================
// Dialog reply selection monitor.
// ============================================================================
struct DialogReplyState {
    void*    listBox;
    short    lastSelection;
    // Fingerprint of the reply SET we last saw. A dialogue keeps one panel and
    // one listbox for its whole run, so the selection index alone cannot tell
    // "player moved the cursor" from "the engine swapped in the next node's
    // replies" — see ComputeReplySetSignature.
    unsigned lastSetSignature;
};
DialogReplyState s_dialogReplyState = { nullptr, -1, 0 };

// One-shot "park the cursor off the reply list" latch, armed when a dialog
// reply panel first appears and cleared once the park is issued (or on disarm).
bool s_dialogReplyParkPending = false;

// FNV-1a over every active reply's text plus the count. Changes whenever the
// engine swaps the reply set, and is stable while the player navigates within
// one set. Returns 0 ("unknown") when the reply store is empty or disagrees
// with the listbox row count — that mismatch is exactly the window where the
// store still holds the PREVIOUS node's replies (or none yet) while the rows
// are already up, so a signature taken there would fingerprint stale text.
unsigned ComputeReplySetSignature(int replyCount, int rowCount) {
    if (replyCount <= 0 || replyCount != rowCount) return 0;
    unsigned h = 2166136261u;
    for (int i = 0; i < replyCount; ++i) {
        char text[256];
        if (!acc::engine::ReadDialogReplyText(i, text, sizeof(text))) return 0;
        for (const char* p = text; *p; ++p) {
            h = (h ^ static_cast<unsigned char>(*p)) * 16777619u;
        }
        h = (h ^ 0xffu) * 16777619u;  // row separator
    }
    h ^= static_cast<unsigned>(replyCount);
    return h ? h : 1u;  // never collide with the "unknown" sentinel
}

bool ParkDialogCursorOffReplies(void* replyLb) {
    // Log the reply-list geometry so a bad park is diagnosable from the field
    // log without a repro on our end.
    auto* ext = reinterpret_cast<int*>(
        reinterpret_cast<unsigned char*>(replyLb) + kControlExtentOffset);
    acclog::Write("Menus.DialogReply", "reply list x=%d y=%d w=%d h=%d",
                  ext[0], ext[1], ext[2], ext[3]);
    return acc::menus::detail::ParkCursorToCorner("Menus.DialogReply");
}

bool IsDialogPanelKind(PanelKind k) {
    switch (k) {
    case PanelKind::DialogCinematic:
    case PanelKind::DialogCinematicCopy:
    case PanelKind::DialogComputer:
    case PanelKind::DialogComputerCamera:
        return true;
    default:
        return false;
    }
}

}  // namespace

void MonitorDialogReplies() {
    constexpr int kCap = 32;
    void* panelData[kCap];
    int panelCount = acc::engine::ReadPanelArray(
        acc::engine::GetGuiManager(), panelData, kCap);

    void* dialogPanel = nullptr;
    PanelKind dialogKind = PanelKind::Unknown;
    for (int i = 0; i < panelCount; ++i) {
        void* p = panelData[i];
        if (!p) continue;
        PanelKind pk = IdentifyPanel(p);
        if (IsDialogPanelKind(pk)) {
            dialogPanel = p;
            dialogKind  = pk;
            break;
        }
    }

    if (!dialogPanel) {
        if (s_dialogReplyState.listBox) {
            acclog::Write("Menus.DialogReply", "monitor disarmed: no dialog panel in stack");
            s_dialogReplyState.listBox = nullptr;
            s_dialogReplyState.lastSelection = -1;
            s_dialogReplyState.lastSetSignature = 0;
            s_dialogReplyParkPending = false;
        }
        return;
    }

    // The reply listbox is a FIXED member of CSWGuiDialog at +0x19c4 in every
    // dialogue variant (cinematic / cinematic-copy / computer / computer-camera),
    // set by the exe's struct layout — NOT by the .gui. We must NOT locate it by
    // "first listbox in controls[]" (FindListBoxChild): the computer/droid panel
    // (CSWGuiDialogComputer) carries a SECOND listbox — the terminal-output
    // LB_MESSAGE — and CSWGuiPanel::InitControl slots controls into controls[] by
    // their .gui layout id, not registration order. On the stock computer.gui the
    // scan happens to work (LB_REPLIES id=11 < LB_MESSAGE id=17, so LB_REPLIES is
    // reached first), which is why this never reproduced in-house. But a UI mod
    // (widescreen/HD packs ship replacement .gui files) that renumbers LB_MESSAGE
    // below LB_REPLIES makes the scan return the terminal listbox instead of the
    // replies — the droid/computer menu then tracks the wrong selection index and
    // reads garbage / only one option. Addressing by struct offset is immune to
    // any layout renumbering. (Verified via decompiled InitControl + the shipped
    // .gui control ids, 2026-07-18.)
    void* lb = reinterpret_cast<unsigned char*>(dialogPanel) +
               kDialogRepliesListBoxOffset;
    if (!acc::engine::IsListBox(lb)) return;
    PanelKind k = dialogKind;
    void* fg = dialogPanel;

    uint16_t selRaw = 0;  // guarded: the dialog panel can close mid-tick
    if (!acc::engine::TryReadU16(lb, kListBoxSelectionIndexOffset, &selRaw))
        return;
    short selIdx = static_cast<short>(selRaw);

    if (s_dialogReplyState.listBox != lb) {
        s_dialogReplyState.listBox = lb;
        s_dialogReplyState.lastSelection = selIdx;
        s_dialogReplyState.lastSetSignature = 0;
        s_dialogReplyParkPending = true;
        // Diagnostic: log what the OLD first-listbox scan WOULD have returned and
        // its offset within the panel. If scanOff != 0x19c4 the reporter is on a
        // renumbered .gui (UI mod) and the pre-fix build was reading the wrong
        // listbox — the root-cause confirmation for the droid/computer menu bug.
        // replyCount shows whether the authoritative reply store is populated.
        void* scanLb = FindListBoxChild(dialogPanel);
        char  scanOff[24];
        if (scanLb) {
            snprintf(scanOff, sizeof(scanOff), "0x%lx",
                     static_cast<unsigned long>(
                         reinterpret_cast<unsigned char*>(scanLb) -
                         reinterpret_cast<unsigned char*>(dialogPanel)));
        } else {
            std::strcpy(scanOff, "none");
        }
        acclog::Write("Menus.DialogReply", "monitor armed: panel=%p kind=%s listbox=%p "
                      "(fixed off=0x%x) scanListbox=%p scanOff=%s replyCount=%d "
                      "initialSel=%d", fg, PanelKindName(k), lb,
                      (unsigned)kDialogRepliesListBoxOffset, scanLb, scanOff,
                      acc::engine::ReadDialogReplyCount(), selIdx);
        return;
    }

    auto* lbList = reinterpret_cast<CExoArrayList*>(
        reinterpret_cast<unsigned char*>(lb) + kListBoxControlsOffset);

    // One-shot cursor park: once the reply list is populated, warp the cursor
    // off it so the engine's hover-select stops overwriting our keyboard-driven
    // selection (see ParkDialogCursorOffReplies). Deferred to here (Update tick)
    // rather than the arm branch so it fires after the rows exist; runs before
    // the selection-change dedup so a stale hover-snap doesn't gate it.
    if (s_dialogReplyParkPending && lbList && lbList->data && lbList->size > 0) {
        if (ParkDialogCursorOffReplies(lb)) {
            s_dialogReplyParkPending = false;
        }
    }

    // A dialogue runs its whole length on ONE panel and ONE reply listbox, so
    // the selection index alone is not enough to notice the next node's replies:
    // the engine selects row 0 for each new node, and lastSelection is usually
    // already 0 from the node before — the change detector sees nothing and the
    // player is left in silence with a reply prompt waiting. Worst case is a
    // node with a SINGLE reply, which can never move off row 0, so it was never
    // announced at all (the player only learns a choice is pending by pressing
    // Enter). Treat a change of the reply SET as an announce event too.
    int replyCount = acc::engine::ReadDialogReplyCount();
    unsigned setSig = ComputeReplySetSignature(
        replyCount, (lbList ? lbList->size : 0));
    bool newSet = setSig != 0 && setSig != s_dialogReplyState.lastSetSignature;
    if (setSig != 0) {
        // Consume the event immediately: whatever the announce path below does
        // with it (speak, defer to a tutorial popup, bail on a stale index), the
        // set has been seen and must not re-fire on every following tick.
        s_dialogReplyState.lastSetSignature = setSig;
    }
    if (newSet) {
        // Re-park on every new reply set, not just on dialogue open: the engine
        // can put the cursor back over the reply list between nodes, and a
        // resting cursor pins the selection to its row via hover-select — every
        // keyboard move snaps straight back before it can be spoken. Seen on
        // the Peragus work assignment console: from the second node on, the
        // selection arrived pinned to "Log out" and Up/Down/Home were all
        // reverted within a tick (userlogs/078multioptionconsole, 17:17:06+).
        // The park lands on the next monitor tick, same deferred path as the
        // open-time park above.
        s_dialogReplyParkPending = true;
    }

    if (selIdx == s_dialogReplyState.lastSelection && !newSet) return;
    short prev = s_dialogReplyState.lastSelection;
    s_dialogReplyState.lastSelection = selIdx;

    if (selIdx < 0) {
        acclog::Write("Menus.DialogReply", "selection cleared: listbox=%p prev=%d",
                      lb, prev);
        return;
    }

    if (!lbList || !lbList->data || selIdx >= lbList->size) {
        acclog::Write("Menus.DialogReply", "selection out of range: listbox=%p sel=%d "
                      "size=%d", lb, selIdx,
                      (lbList ? lbList->size : -1));
        return;
    }

    void* row = lbList->data[selIdx];

    // Authoritative source: the engine's render-independent reply-text array
    // on CGuiInGame (populated by SetReplyData per active reply). The reply
    // listbox rows are a scrolling list whose CSWGuiLabel text materialises
    // only for on-page rows, so reading the row directly silently drops
    // off-page replies (the "missing entries" bug in droid/computer
    // interfaces). Fall back to the row label only if the state read misses.
    char text[256];
    const char* src = nullptr;
    if (acc::engine::ReadDialogReplyText(selIdx, text, sizeof(text)) &&
        text[0] != '\0') {
        src = "dialog-state";
    } else if (row) {
        src = acc::menus::extract::FromControl(row, text, sizeof(text));
    }
    if (src) {
        // src=row-label (not dialog-state) means ReadDialogReplyText missed and
        // we fell back to the on-page row label — the paging-drop failure mode.
        // size vs replyCount exposes a listbox/reply-array count mismatch.
        // newSet=1 means the announcement was triggered by the reply set
        // changing (a new dialogue node) rather than by the player navigating.
        acclog::Write("Menus.DialogReply", "selected: panel=%p kind=%s listbox=%p "
                      "sel=%d (was %d) src=%s size=%d replyCount=%d newSet=%d "
                      "text=\"%s\"",
                      fg, PanelKindName(k), lb, selIdx, prev, src, lbList->size,
                      replyCount, newSet ? 1 : 0, text);
        // A reply is now readable/navigable — the entry's VO has ended and the
        // player can choose. This is the moment to pop the pending tutorial
        // hint (not at VO start: that talked over Trask and stole Enter-as-skip).
        // If it fired, skip re-speaking this reply so it doesn't overlap the
        // popup; the player hears the reply when they navigate after dismissing.
        if (acc::tutorial_popup::FirePendingAtReplyBreak()) return;
        // Sole speaker for dialogue replies (the input path drives the cursor
        // silently — see the no-announce note on the dialog specs in
        // menus_listbox.cpp). Append the "%d of %d" position so the user knows
        // how many replies there are.
        char msg[320];
        snprintf(msg, sizeof(msg),
                 acc::strings::Get(acc::strings::Id::FmtContainerItemAt),
                 text, selIdx + 1, lbList->size);
        acc::speech::Speak(msg, /*interrupt=*/false);
    } else {
        char vtbl[160];
        if (row) DumpControlVtable(row, vtbl, sizeof(vtbl));
        else     snprintf(vtbl, sizeof(vtbl), "row=NULL");
        acclog::Write("Menus.DialogReply", "selected (src=none): panel=%p listbox=%p "
                      "sel=%d row=%p %s", fg, lb, selIdx, row, vtbl);
    }
}

}  // namespace acc::menus::monitors

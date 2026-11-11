// The container screen (loot, give items): its list box, Enter and Escape,
// the selection monitor and the give-mode key.

#include "menus_listbox_internal.h"

#include "engine_manager.h"
#include "engine_offsets.h"
#include "engine_panels.h"
#include "engine_reads.h"
#include "hotkeys.h"
#include "log.h"
#include "menus_extract.h"
#include "menus_pending.h"
#include "speech.h"

namespace acc::menus::listbox {

namespace {

// ============================================================================
// Subsystem-paired monitors. Each watches state owned by one of the spec
// entries above and announces changes per tick. Co-located here as of the
// post-Step-5 cleanup so state + handler + monitor live in one TU. Called
// from TickListboxMonitors below, which is fanned out from menus.cpp's
// TickMonitors.
// ============================================================================
struct ContainerSelState {
    void* listBox;
    short lastSelection;
};
ContainerSelState s_containerSelState = { nullptr, -1 };

// ============================================================================
// Container — loot chest / corpse panel ("PlÃ¼ndern").
// ============================================================================

bool ContainerMatches(void* p) {
    return IdentifyPanel(p) == PanelKind::Container;
}

void* ContainerFindLb(void* p) {
    return ContainerPanelItemsListBox(p);
}

// Inline announce only on a no-op clamp (boundary). Normal moves are
// caught by MonitorContainerSelection on the next tick.
void ContainerAnnounce(void* /*lb*/, const ListBoxNavResult& r) {
    if (r.newSel != r.oldSel) return;
    if (!r.row) return;
    char rowText[256];
    if (acc::menus::extract::FromControl(r.row, rowText, sizeof(rowText))) {
        char msg[320];
        snprintf(msg, sizeof(msg),
                 acc::strings::Get(acc::strings::Id::FmtContainerItemAt),
                 rowText, r.newSel + 1, r.rowCount);
        acc::speech::Speak(msg, /*interrupt=*/false);
        // Append stack count when the row holds a stackable item with
        // more than one copy. Stays silent on stack_size <= 1 so weapons
        // / armour don't drag "1 StÃ¼ck" through every announce.
        int stack = acc::engine::ReadItemRowStackCount(r.row);
        if (stack > 1) {
            char suffix[64];
            snprintf(suffix, sizeof(suffix),
                     acc::strings::Get(acc::strings::Id::FmtItemStackSuffix),
                     stack);
            acc::speech::Speak(suffix, /*interrupt=*/false);
        }
        int charges = acc::engine::ReadItemRowCharges(r.row);
        if (charges >= 0) {
            char suffix[64];
            snprintf(suffix, sizeof(suffix),
                     acc::strings::Get(acc::strings::Id::FmtItemChargeSuffix),
                     charges);
            acc::speech::Speak(suffix, /*interrupt=*/false);
        }
    }
}

bool ContainerOnEnter(void* panel) {
    // Container per-item take is currently UNRESOLVED. Both tested
    // primitives fail:
    //   * vtable[15] FireActivate on the row → engine doesn't translate
    //     to "take this row" — rowCount stays unchanged.
    //   * Click-sim at row.GetControlCenter() coords → cursor hits dead
    //     space (Down=0, Up=0). Row extents are listbox-local, not
    //     screen-absolute; we'd need parent offset accumulation.
    //
    // Until we identify the engine's row-take primitive (likely embedded
    // in CSWGuiContainer::HandleInputEvent at 0x006b92f0 or the protoitem's
    // onClick), Enter dispatches BTN_OK unconditionally — the working
    // "take-all" gesture. Per-item take = lost feature, deferred. See
    // docs/equip-flow-investigation.md for the parallel investigation
    // that landed the same shape on equip.
    QueueControlActivate(ContainerPanelOkButton(panel),
                         "Container: Enter -> BTN_OK (take-all; "
                         "per-item take deferred)");
    return true;
}

bool ContainerOnEsc(void* panel) {
    QueueControlActivate(ContainerPanelCancelButton(panel),
                         "Container: Esc -> BTN_CANCEL");
    return true;
}

}  // namespace

constexpr ListBoxPanelSpec kContainerSpec = {
    /*logTag*/                  "Container",
    /*matches*/                 ContainerMatches,
    /*armed*/                   nullptr,
    /*resetStale*/              nullptr,
    /*findListBox*/             ContainerFindLb,
    /*minSel*/                  0,
    /*announce*/                ContainerAnnounce,
    /*enrichRow*/               nullptr,
    /*logExtra*/                nullptr,
    /*onEnter*/                 ContainerOnEnter,
    /*onEsc*/                   ContainerOnEsc,
    /*titleOverride*/           nullptr,
    /*emptyStateId*/            acc::strings::Id::Count_,  // empty containers don't open at all
    /*alwaysReturnFromHandler*/ true,
};

void MonitorContainerSelection() {
    void* containerPanel = FindPanelByKind(PanelKind::Container);

    if (!containerPanel) {
        if (s_containerSelState.listBox) {
            acclog::Write("Menus.Container", "monitor disarmed: no Container panel in stack");
            s_containerSelState.listBox = nullptr;
            s_containerSelState.lastSelection = -1;
        }
        return;
    }

    void* lb = ContainerPanelItemsListBox(containerPanel);
    if (!lb) return;

    const int rowCount = acc::engine::ReadListBoxRowCount(lb);
    const short selIdx = static_cast<short>(acc::engine::ReadListBoxSelection(lb));

    if (s_containerSelState.listBox != lb) {
        s_containerSelState.listBox       = lb;
        s_containerSelState.lastSelection = selIdx;
        if (rowCount <= 0) {
            acc::speech::Speak(acc::strings::Get(acc::strings::Id::ContainerEmpty),
                        /*interrupt=*/false);
            acclog::Write("Menus.Container", "monitor armed: panel=%p lb=%p empty initialSel=%d",
                          containerPanel, lb, selIdx);
        } else if (rowCount == 1) {
            acc::speech::Speak(acc::strings::Get(acc::strings::Id::ContainerOneItem),
                        /*interrupt=*/false);
            acclog::Write("Menus.Container", "monitor armed: panel=%p lb=%p count=1 initialSel=%d",
                          containerPanel, lb, selIdx);
        } else {
            char msg[64];
            snprintf(msg, sizeof(msg),
                     acc::strings::Get(acc::strings::Id::FmtContainerItems),
                     rowCount);
            acc::speech::Speak(msg, /*interrupt=*/false);
            acclog::Write("Menus.Container", "monitor armed: panel=%p lb=%p count=%d initialSel=%d",
                          containerPanel, lb, rowCount, selIdx);
        }
        return;
    }

    if (selIdx == s_containerSelState.lastSelection) return;
    short prev = s_containerSelState.lastSelection;
    s_containerSelState.lastSelection = selIdx;

    if (selIdx < 0) {
        acclog::Write("Menus.Container", "selection cleared: lb=%p prev=%d", lb, prev);
        return;
    }
    void* row = acc::engine::ReadListBoxRow(lb, selIdx);
    if (!row) {
        acclog::Write("Menus.Container", "no row at selection: lb=%p sel=%d size=%d",
                      lb, selIdx, rowCount);
        return;
    }

    char rowText[256];
    const char* src = acc::menus::extract::FromControl(row, rowText, sizeof(rowText));
    if (!src) {
        acclog::Write("Menus.Container", "row %d (lb=%p) no announceable text", selIdx, lb);
        return;
    }

    char msg[320];
    snprintf(msg, sizeof(msg),
             acc::strings::Get(acc::strings::Id::FmtContainerItemAt),
             rowText, selIdx + 1, rowCount);
    acc::speech::Speak(msg, /*interrupt=*/false);
    int stack = acc::engine::ReadItemRowStackCount(row);
    if (stack > 1) {
        char suffix[64];
        snprintf(suffix, sizeof(suffix),
                 acc::strings::Get(acc::strings::Id::FmtItemStackSuffix),
                 stack);
        acc::speech::Speak(suffix, /*interrupt=*/false);
    }
    int charges = acc::engine::ReadItemRowCharges(row);
    if (charges >= 0) {
        char suffix[64];
        snprintf(suffix, sizeof(suffix),
                 acc::strings::Get(acc::strings::Id::FmtItemChargeSuffix),
                 charges);
        acc::speech::Speak(suffix, /*interrupt=*/false);
    }
    acclog::Write("Menus.Container", "row lb=%p sel=%d (was %d) text=\"%s\" stack=%d charges=%d",
                  lb, selIdx, prev, rowText, stack, charges);
}

void PollContainerGiveModeKey() {
    if (!acc::hotkeys::Pressed(acc::hotkeys::Action::ContainerGiveMode)) return;

    void* mgr = *reinterpret_cast<void**>(kAddrGuiManagerPtr);
    if (!mgr) return;
    void* fgPanel = acc::engine::GetForegroundPanel(mgr);
    if (!fgPanel || IdentifyPanel(fgPanel) != PanelKind::Container) return;

    if (acc::menus::pending::IsPending()) {
        acclog::Write("Container", "G (give-mode) -- op already pending; ignoring");
        return;
    }
    void* btn = ContainerPanelGiveItemsButton(fgPanel);
    if (!btn) {
        acclog::Write("Container", "G (give-mode) -- BTN_GIVEITEMS not found on panel=%p",
                      fgPanel);
        return;
    }
    acc::menus::pending::QueueActivate(btn);
    acclog::Write("Container", "G (give-mode) -> FireActivate BTN_GIVEITEMS panel=%p target=%p",
                  fgPanel, btn);
}

}  // namespace acc::menus::listbox

// Left/Right on the chain cursor (sliders, cycle arrows) and Esc (back out of popups and
// sub-screens).

#include "menus_chain.h"

#include "engine_input.h"
#include "engine_offsets.h"
#include "engine_panels.h"
#include "engine_reads.h"
#include "log.h"
#include "menus_internal.h"
#include "menus_listbox.h"
#include "menus_monitors.h"
#include "menus_pending.h"
#include "menus_store.h"

#include <cstdint>

namespace acc::menus::chain {

void HandleLeftRight(void* activePanel, int code, int val, bool& consumed) {
    if (val == 0) return;
    if (code != kInputNavLeft && code != kInputNavRight) return;
    // Pazaak wager popup owns Left/Right via pazaak::Tick's polled hold-to-
    // repeat stepper. Consume here so the slider/cycle-arrow path can't also
    // act (FindAdjacentArrow would otherwise re-fire the masked less/more
    // button, double-stepping the wager).
    if (activePanel &&
        acc::engine::IdentifyPanel(activePanel) == acc::engine::PanelKind::PazaakWager) {
        consumed = true;
        return;
    }
    if (activePanel == nullptr || g_chainPanel != activePanel) return;
    if (g_chainCount <= 0 || g_chainIndex < 0 || g_chainIndex >= g_chainCount) return;

    void* focused = g_chain[g_chainIndex].control;
    bool toRight = (code == kInputNavRight);

    if (acc::engine::IsSlider(focused)) {
        if (acc::menus::pending::IsPending()) {
            acclog::Write("Menus.Slider", "%s: op already pending; ignoring",
                          toRight ? "right" : "left");
        } else {
            int sliderCode = toRight ? 500 : 501;
            acc::menus::pending::QueueSliderInput(focused, sliderCode);
            acclog::Write("Menus.Slider", "%s panel=%p focus=%p code=%d",
                          toRight ? "right" : "left",
                          activePanel, focused, sliderCode);
        }
    } else {
        // Panel-aware cycle override: in CSWGuiPortraitCharGen the chain
        // holds left_arrow as the lone anchor (right_arrow is filtered out
        // in RebindChain). FindAdjacentArrow can pick up the right_arrow
        // as a same-row neighbour when going right, but going left there's
        // nothing to the left of x=272 — so we resolve the targets directly
        // from the panel offsets:
        //   Left  → activate left_arrow (cycles -1)
        //   Right → activate right_arrow (cycles +1)
        // Engine's UpdatePortraitButton writes the new resref to
        // creature.portrait, the per-frame focus monitor re-reads, and
        // the diff fires the new "Portrait: …" announcement.
        void* portraitTarget = nullptr;
        {
            void** pVt = *reinterpret_cast<void***>(activePanel);
            if (reinterpret_cast<uintptr_t>(pVt) ==
                    kVtableCSWGuiPortraitCharGen) {
                auto* base = reinterpret_cast<unsigned char*>(activePanel);
                void* leftArrow = base + kPortraitLeftArrowOffset;
                if (focused == leftArrow) {
                    portraitTarget = toRight
                        ? (void*)(base + kPortraitRightArrowOffset)
                        : leftArrow;
                }
            }
        }
        void* neighbor = portraitTarget
            ? portraitTarget
            : FindAdjacentArrow(activePanel, focused, toRight);
        if (neighbor) {
            if (acc::menus::pending::IsPending()) {
                acclog::Write("Menus.Cycle", "%s: op already pending; ignoring",
                              toRight ? "right" : "left");
            } else {
                acc::menus::pending::QueueActivate(neighbor);
                acclog::Write("Menus.Cycle", "%s panel=%p focus=%p neighbor=%p%s",
                              toRight ? "right" : "left",
                              activePanel, focused, neighbor,
                              portraitTarget ? " (portrait-anchor)" : "");
            }
        } else {
            acclog::Write("Menus.Cycle", "%s: no adjacent arrow for focus=%p",
                          toRight ? "right" : "left", focused);
        }
    }
    consumed = true;
}

void HandleEsc(void* activePanel, int code, int val, bool& consumed) {
    if (val == 0) return;
    if (code != kInputEsc1 && code != kInputEsc2) return;

    // Store-specific Esc: route to cancel_button (Close) directly.
    // The store isn't in IsModalPopupPanel (it's the foreground modal,
    // not a popup on top), and the chain doesn't include the cancel
    // button anymore (we filter it out so it doesn't clutter Up/Down
    // nav), so without this Esc would no-op on the store.
    if (acc::menus::store::IsStorePanel(activePanel)) {
        if (acc::menus::store::CloseFromEsc()) consumed = true;
    }

    // Workbench upgrade panel Esc: route to BTN_BACK ("Cancel") directly
    // (K1 upgrade.gui id 28; K2 upgrade_p.gui renumbers it to 13). Same
    // shape as the store branch above — the upgrade.gui panel is the
    // foreground modal (not a popup on top), so the generic Esc gate below
    // (IsModalPopupPanel / g_tabbedPanel / escIsOptionsSub) doesn't fire.
    // We also can't rely on FindCancelButton landing on BTN_BACK reliably
    // here (see kWorkbenchUpgradeSpec comments).
    // While the picker is armed the spec's onEsc disarms; this branch
    // only catches Esc when the picker is NOT armed (user on a slot
    // button, BTN_ASSEMBLE, or BTN_BACK).
    if (!consumed && activePanel != nullptr &&
        acc::engine::IdentifyPanel(activePanel) ==
            acc::engine::PanelKind::WorkbenchUpgrade &&
        !acc::menus::listbox::IsWorkbenchUpgradePickerArmed())
    {
        if (acc::menus::pending::IsPending()) {
            acclog::Write("Esc", "WorkbenchUpgrade — op already pending; ignoring");
            consumed = true;
        } else {
            void* back =
                acc::menus::detail::UpgradePanelBackButton(activePanel);
            if (back) {
                acc::menus::pending::QueueActivate(back);
                acclog::Write("Esc",
                              "WorkbenchUpgrade -> BTN_BACK panel=%p target=%p",
                              activePanel, back);
                consumed = true;
            } else {
                acclog::Write("Esc",
                              "WorkbenchUpgrade -- BTN_BACK not found on panel=%p",
                              activePanel);
            }
        }
    }

    // InGameOptions sub-screen override: the parent strip's controls[0] is
    // a button (Load Game), not a listbox, so DetectTabsCluster never
    // latches and `g_tabbedPanel` stays null — the tabbed-parent arm above
    // misses every in-game Options sub-screen. The foreground may also be
    // a HUD layer rather than the sub-screen itself, depending on overlay
    // ordering, so activePanel isn't a reliable target either.
    //
    // `g_chainPanel` is the right discriminator: SetActiveControl re-binds
    // the chain to the heap-allocated sub-screen on entry, so it points at
    // Gameplay / Graphics / Sound / Auto-Pause / Feedback /
    // Keymapping / Mouse settings for the lifetime of that screen,
    // and FindCloseButton on it resolves Close reliably. The
    // IsInGameOptionsSubScreen helper already excludes the parent strip.
    void* escTargetPanel = activePanel;
    bool  escIsOptionsSub = false;
    if (g_chainPanel != nullptr &&
        acc::engine::IsInGameOptionsSubScreen(g_chainPanel))
    {
        escTargetPanel = g_chainPanel;
        escIsOptionsSub = true;
    }

    if (escTargetPanel != nullptr &&
        ((g_tabbedPanel != nullptr && escTargetPanel != g_tabbedPanel) ||
         acc::engine::IsModalPopupPanel(
             acc::engine::IdentifyPanel(escTargetPanel)) ||
         escIsOptionsSub))
    {
        if (acc::menus::pending::IsPending()) {
            acclog::Write("Esc", "op already pending; ignoring");
            consumed = true;
        } else {
            // Probe order matters: confirm-style popups (OK + Cancel,
            // Yes + No, …) carry BOTH a cancel-intent button AND the
            // affirmative that FindCloseButton matches as "OK". Esc is a
            // back-out gesture, never a confirm — try Cancel/Cancel
            // first so the quit-confirm and save-overwrite-style dialogs
            // route Esc to the safe choice. Single-button info popups
            // (StatusSummary's lone Close, AreaTransition's Continue)
            // have no cancel button, so the FindCloseButton fallback
            // handles them.
            void* cancelBtn = FindCancelButton(escTargetPanel);
            void* tgt = cancelBtn ? cancelBtn : FindCloseButton(escTargetPanel);
            if (tgt) {
                acc::menus::pending::QueueActivate(tgt);
                // InGameOptions sub-screens: the close fires a deferred
                // destroy — the engine keeps the panel in panels[] across
                // the FireActivate dispatch (ValidateChainPanel finds it,
                // chain stays), then frees the panel + children at end
                // of tick. Between those two ticks, MonitorFocusedControl
                // walks g_chain[g_chainIndex].control, dereferences a
                // freed button, and FromControl's SEH-caught AV interacts
                // with /GS to fastfail. Confirmed by crash dump TID 16116:
                // ESI matched the chain entry the user had last navigated
                // to before pressing Esc.
                //
                // ValidateChainPanel can't help (panel still in panels[]
                // when it runs), and chain[10] nulling only covers
                // Close itself — the other 11 entries are equally
                // dead. Invalidate the whole chain here; the Close
                // pointer is already captured by QueueActivate and the
                // next SetActiveControl rebuilds against whatever the
                // engine refocuses on.
                if (escIsOptionsSub) InvalidateChain();
                acclog::Write("Menus.Esc",
                              "%s panel=%p kind=%s target=%p%s",
                              cancelBtn ? "cancel" : "close",
                              escTargetPanel,
                              acc::engine::PanelKindName(
                                  acc::engine::IdentifyPanel(escTargetPanel)),
                              tgt,
                              escIsOptionsSub
                                  ? " (InGameOptions sub-screen)" : "");
                consumed = true;
            } else {
                acclog::Write("Menus.Esc",
                              "sub-dialog panel=%p kind=%s but no cancel/close "
                              "button found; passing through",
                              escTargetPanel,
                              acc::engine::PanelKindName(
                                  acc::engine::IdentifyPanel(escTargetPanel)));
            }
        }
    }
}

}  // namespace acc::menus::chain

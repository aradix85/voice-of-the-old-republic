// Left/Right on the chain cursor (sliders, cycle arrows) and Esc (back out of
// popups and sub-screens).

#include "menus_chain.h"

#include "engine_input.h"
#include "engine_offsets.h"
#include "engine_panels.h"
#include "engine_reads.h"
#include "log.h"
#include "menus_internal.h"
#include "menus_listbox.h"
#include "menus_pending.h"
#include "menus_store.h"

#include <cstdint>

namespace acc::menus::chain {

namespace {

// Character creation's portrait picker keeps only its left arrow in the chain,
// and nothing lies left of it for FindAdjacentArrow: on that arrow, Left and
// Right activate the left and right arrow directly. The focus monitor then
// announces the new portrait.
void* PortraitArrowTarget(void* panel, void* focused, bool toRight) {
    void** vt = *reinterpret_cast<void***>(panel);
    if (reinterpret_cast<uintptr_t>(vt) != kVtableCSWGuiPortraitCharGen) return nullptr;
    auto* base = reinterpret_cast<unsigned char*>(panel);
    void* leftArrow = base + kPortraitLeftArrowOffset;
    if (focused != leftArrow) return nullptr;
    return toRight ? static_cast<void*>(base + kPortraitRightArrowOffset) : leftArrow;
}

// The upgrade screen is the foreground panel, not a popup, so the generic Esc
// below does not see it: Esc goes to BTN_BACK. While its picker is armed the
// picker's own Esc disarms it instead. False when BTN_BACK is not found.
bool EscWorkbenchUpgrade(void* panel) {
    if (acc::menus::pending::IsPending()) {
        acclog::Write("Esc", "WorkbenchUpgrade — op already pending; ignoring");
        return true;
    }
    void* back = acc::menus::detail::UpgradePanelBackButton(panel);
    if (!back) {
        acclog::Write("Esc", "WorkbenchUpgrade -- BTN_BACK not found on panel=%p", panel);
        return false;
    }
    acc::menus::pending::QueueActivate(back);
    acclog::Write("Esc", "WorkbenchUpgrade -> BTN_BACK panel=%p target=%p", panel, back);
    return true;
}

// Esc on a popup, a tabbed sub-dialog or an in-game Options sub-screen:
// activate its Cancel, or else its Close/OK/Continue. Cancel comes first so a
// confirm dialog takes the safe choice; Esc is never a confirm.
void EscPopup(void* panel, bool isOptionsSub, bool& consumed) {
    if (acc::menus::pending::IsPending()) {
        acclog::Write("Esc", "op already pending; ignoring");
        consumed = true;
        return;
    }
    const char* kindName = acc::engine::PanelKindName(acc::engine::IdentifyPanel(panel));
    void* cancelBtn = FindCancelButton(panel);
    void* target = cancelBtn ? cancelBtn : FindCloseButton(panel);
    if (!target) {
        acclog::Write("Menus.Esc", "sub-dialog panel=%p kind=%s but no cancel/close "
                      "button found; passing through", panel, kindName);
        return;
    }
    acc::menus::pending::QueueActivate(target);
    // An Options sub-screen is freed at the end of the tick that closes it,
    // while still listed in the manager, so every chain entry would dangle:
    // drop the chain now (the target is already captured).
    if (isOptionsSub) InvalidateChain();
    acclog::Write("Menus.Esc", "%s panel=%p kind=%s target=%p%s",
                  cancelBtn ? "cancel" : "close", panel, kindName, target,
                  isOptionsSub ? " (InGameOptions sub-screen)" : "");
    consumed = true;
}

}  // namespace

void HandleLeftRight(void* activePanel, int code, int val, bool& consumed) {
    if (val == 0) return;
    if (code != kInputNavLeft && code != kInputNavRight) return;
    // The Pazaak wager popup steps on Left/Right in pazaak::Tick (hold to
    // repeat); acting here as well would step twice.
    if (activePanel &&
        acc::engine::IdentifyPanel(activePanel) == acc::engine::PanelKind::PazaakWager) {
        consumed = true;
        return;
    }
    if (activePanel == nullptr || g_chainPanel != activePanel) return;
    if (g_chainCount <= 0 || g_chainIndex < 0 || g_chainIndex >= g_chainCount) return;

    void* focused = g_chain[g_chainIndex].control;
    const bool toRight = (code == kInputNavRight);
    const char* dir = toRight ? "right" : "left";
    consumed = true;

    if (acc::engine::IsSlider(focused)) {
        if (acc::menus::pending::IsPending()) {
            acclog::Write("Menus.Slider", "%s: op already pending; ignoring", dir);
            return;
        }
        const int sliderCode = toRight ? 500 : 501;
        acc::menus::pending::QueueSliderInput(focused, sliderCode);
        acclog::Write("Menus.Slider", "%s panel=%p focus=%p code=%d",
                      dir, activePanel, focused, sliderCode);
        return;
    }

    void* portraitTarget = PortraitArrowTarget(activePanel, focused, toRight);
    void* neighbor = portraitTarget ? portraitTarget
                                    : FindAdjacentArrow(activePanel, focused, toRight);
    if (!neighbor) {
        acclog::Write("Menus.Cycle", "%s: no adjacent arrow for focus=%p", dir, focused);
        return;
    }
    if (acc::menus::pending::IsPending()) {
        acclog::Write("Menus.Cycle", "%s: op already pending; ignoring", dir);
        return;
    }
    acc::menus::pending::QueueActivate(neighbor);
    acclog::Write("Menus.Cycle", "%s panel=%p focus=%p neighbor=%p%s",
                  dir, activePanel, focused, neighbor,
                  portraitTarget ? " (portrait-anchor)" : "");
}

void HandleEsc(void* activePanel, int code, int val, bool& consumed) {
    if (val == 0) return;
    if (code != kInputEsc1 && code != kInputEsc2) return;

    // The store is the foreground panel, not a popup, and its Close is kept out
    // of the chain: Esc closes it directly.
    if (acc::menus::store::IsStorePanel(activePanel) &&
        acc::menus::store::CloseFromEsc()) {
        consumed = true;
        return;
    }
    if (activePanel != nullptr &&
        acc::engine::IdentifyPanel(activePanel) == acc::engine::PanelKind::WorkbenchUpgrade &&
        !acc::menus::listbox::IsWorkbenchUpgradePickerArmed() &&
        EscWorkbenchUpgrade(activePanel)) {
        consumed = true;
        return;
    }

    // An in-game Options sub-screen: the foreground may be a HUD layer and the
    // parent strip has no tab cluster, but the chain is bound to the
    // sub-screen itself.
    const bool isOptionsSub =
        g_chainPanel != nullptr && acc::engine::IsInGameOptionsSubScreen(g_chainPanel);
    void* target = isOptionsSub ? g_chainPanel : activePanel;
    if (target == nullptr) return;
    if (isOptionsSub ||
        (g_tabbedPanel != nullptr && target != g_tabbedPanel) ||
        acc::engine::IsModalPopupPanel(acc::engine::IdentifyPanel(target))) {
        EscPopup(target, isOptionsSub, consumed);
    }
}

}  // namespace acc::menus::chain

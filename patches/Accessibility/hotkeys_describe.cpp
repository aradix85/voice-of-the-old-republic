// Names for actions and keys, and the spoken description of a binding.

#include "hotkeys_internal.h"

#include <windows.h>

#include <cstdio>

namespace acc::hotkeys {

namespace {

// Indexed by Action; the names are also the keys of the saved rebinds.
const char* const kActionNames[] = {
    "InteractTarget",
    "InteractForceRadial",
    "InteractForceRadialSecondary",
    "TargetKey1",
    "TargetKey2",
    "TargetKey3",
    "PersonalKey1",
    "PersonalKey2",
    "PersonalKey3",
    "PersonalKey4",
    "PersonalKey5",
    "PersonalKey6",
    "ActionBarOpen1",
    "ActionBarOpen2",
    "ActionBarOpen3",
    "ActionBarOpen4",
    "ActionBarOpen5",
    "ActionBarOpen6",
    "TargetActionOpen1",
    "TargetActionOpen2",
    "TargetActionOpen3",
    "LevelUpOpen",
    "ExamineOpen",
    "CombatQueueOpen",
    "SelfStatusAnnounce",
    "NavUp",
    "NavDown",
    "NavLeft",
    "NavRight",
    "NavHome",
    "NavEnd",
    "SubmenuEsc",
    "QueueClearAll",
    "ContainerGiveMode",
    "StoreModeToggle",
    "CraftViewToggle",
    "CycleItemPrev",
    "CycleCategoryPrev",
    "CycleItemNext",
    "CycleCategoryNext",
    "CycleItemFirst",
    "CycleItemLast",
    "AnnounceFocus",
    "PathfindFocus",
    "PathfindFocusForce",
    "RouteFocus",
    "AnnounceDegrees",
    "PartyLeaderAnnounce",
    "CameraOrient",
    "SaveMarkerAtCursor",
    "ExploreOverview",
    "ExploreWalkUnexplored",
    "ExploreWalkLoot",
    "PartyStatusAnnounce",
    "EditboxReReadUp",
    "EditboxReReadDown",
    "EditboxSubmit",
    "EditboxCancel",
    "HelpMenuOpen",
    "HelpContext",
    "PazaakStand",
    "PazaakEndTurn",
    "PazaakReviewHand",
    "PazaakReviewTable",
    "PazaakNextCard",
    "PazaakPrevCard",
    "PazaakPlay",
    "PazaakOptLeft",
    "PazaakOptRight",
    "PazaakCancel",
    "PazaakOppHand",
    "TurretCyclePrev",
    "TurretCycleNext",
    "SwoopShift",
    "DialogRepeatLine",
    "CopyLastSpoken",
};
static_assert(sizeof(kActionNames) / sizeof(kActionNames[0]) ==
                  static_cast<size_t>(Action::COUNT),
              "kActionNames needs one name per Action");

// A small rotating pool, so several Describe() results can share a log line.
char g_describeBufs[4][32];
int  g_describeNext = 0;

}  // namespace

const char* Name(Action a) {
    const int idx = static_cast<int>(a);
    if (idx < 0 || idx >= static_cast<int>(Action::COUNT)) return "?";
    return kActionNames[idx];
}

const char* VkLabel(int vk) {
    switch (vk) {
    case 0:             return "(unbound)";
    case VK_RETURN:     return "Enter";
    case VK_ESCAPE:     return "Esc";
    case VK_TAB:        return "Tab";
    case VK_UP:         return "Up";
    case VK_DOWN:       return "Down";
    case VK_LEFT:       return "Left";
    case VK_RIGHT:      return "Right";
    case VK_HOME:       return "Home";
    case VK_END:        return "End";
    case VK_SPACE:      return "Space";
    case VK_OEM_COMMA:  return ",";
    case VK_OEM_PERIOD: return ".";
    case VK_OEM_MINUS:  return "-";
    case VK_OEM_2:      return "/";
    case VK_OEM_3:      return "`";
    case VK_RMENU:      return "AltGr";
    case VK_LMENU:      return "LAlt";
    case VK_MENU:       return "Alt";
    case VK_LCONTROL:   return "LCtrl";
    case VK_RCONTROL:   return "RCtrl";
    case VK_CONTROL:    return "Ctrl";
    case VK_LSHIFT:     return "LShift";
    case VK_RSHIFT:     return "RShift";
    case VK_SHIFT:      return "Shift";
    default: break;
    }
    // Digits and letters are their own VK codes. The buffer holds until the
    // next call.
    static thread_local char buf[4];
    if ((vk >= '0' && vk <= '9') || (vk >= 'A' && vk <= 'Z')) {
        buf[0] = static_cast<char>(vk);
        buf[1] = '\0';
        return buf;
    }
    if (vk >= VK_F1 && vk <= VK_F12) {
        snprintf(buf, sizeof(buf), "F%d", vk - VK_F1 + 1);
        return buf;
    }
    return "?";
}

const char* Describe(Action a) {
    const Binding b = Get(a);
    char* out = g_describeBufs[g_describeNext];
    g_describeNext = (g_describeNext + 1) % 4;
    const size_t cap = sizeof(g_describeBufs[0]);

    // AltGr is named instead of Alt when both are required.
    const uint32_t req = b.modsRequired;
    const char* alt = (req & kModAltGr) ? "AltGr+" : (req & kModAlt) ? "Alt+" : "";
    int n = snprintf(out, cap, "%s%s%s%s", (req & kModCtrl) ? "Ctrl+" : "", alt,
                     (req & kModShift) ? "Shift+" : "", VkLabel(b.vk));
    if (b.altVk != 0 && n > 0 && static_cast<size_t>(n) < cap) {
        snprintf(out + n, cap - n, " / %s", VkLabel(b.altVk));
    }
    return out;
}

}  // namespace acc::hotkeys

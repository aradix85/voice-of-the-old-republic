// Names for actions and keys, and the spoken description of a binding.

#include "hotkeys_internal.h"

namespace acc::hotkeys {

namespace {

// Per-Action name strings for logs / future rebind UI. One slot per Action;
// must be kept in sync with the enum. Lined up vertically so a missing row
// is visually obvious.
const char* const kActionNames[static_cast<int>(Action::COUNT)] = {
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
    "ViewModeToggle",
    "CameraStateProbe",
    "EditboxReReadUp",
    "EditboxReReadDown",
    "EditboxSubmit",
    "EditboxCancel",
    "HelpMenuOpen",
    "HelpContext",
    "ProbeAudioCycle",
    "ProbeAudioFire",
    "ProbeCameraDump",
    "ProbeCameraDistDump",
    "ProbeCameraDistClampToggle",
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

// Rotating buffer pool so several Describe() calls in one log line can
// coexist. 4 slots × 32 chars covers "Shift+Ctrl+AltGr+OEM_MINUS" easily.
char  g_describeBufs[4][32];
int   g_describeNext = 0;

}  // namespace

// ----- Names + descriptions -------------------------------------------------

const char* Name(Action a) {
    int idx = static_cast<int>(a);
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
    case VK_F1:         return "F1";
    case VK_F2:         return "F2";
    case VK_F3:         return "F3";
    case VK_F4:         return "F4";
    case VK_F5:         return "F5";
    case VK_F6:         return "F6";
    case VK_F7:         return "F7";
    case VK_F8:         return "F8";
    case VK_F9:         return "F9";
    case VK_F10:        return "F10";
    case VK_F11:        return "F11";
    case VK_F12:        return "F12";
    default: break;
    }
    // Printable ASCII range that VK codes match directly (A-Z, 0-9).
    if (vk >= '0' && vk <= '9') {
        static thread_local char digit[2] = {0, 0};
        digit[0] = static_cast<char>(vk);
        return digit;
    }
    if (vk >= 'A' && vk <= 'Z') {
        static thread_local char letter[2] = {0, 0};
        letter[0] = static_cast<char>(vk);
        return letter;
    }
    return "?";
}

const char* Describe(Action a) {
    Binding b = Get(a);
    char* out = g_describeBufs[g_describeNext];
    g_describeNext = (g_describeNext + 1) % 4;
    out[0] = '\0';

    auto appendStr = [&](const char* s) {
        size_t cur = std::strlen(out);
        size_t rem = sizeof(g_describeBufs[0]) - cur;
        if (rem <= 1) return;
        std::strncat(out, s, rem - 1);
    };

    // AltGr is a special modifier name — emit it instead of "Alt" when set.
    // When both kModAlt and kModAltGr are required (rare), prefer AltGr.
    if (b.modsRequired & kModCtrl)  appendStr("Ctrl+");
    if (b.modsRequired & kModAltGr) appendStr("AltGr+");
    else if (b.modsRequired & kModAlt) appendStr("Alt+");
    if (b.modsRequired & kModShift) appendStr("Shift+");

    appendStr(VkLabel(b.vk));
    if (b.altVk != 0) {
        appendStr(" / ");
        appendStr(VkLabel(b.altVk));
    }
    return out;
}

}  // namespace acc::hotkeys

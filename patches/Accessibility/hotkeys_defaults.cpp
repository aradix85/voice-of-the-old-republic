// The factory key bindings, one per Action. Several actions share a key; their
// modifier masks, or the panel their handlers gate on, keep exactly one of them
// firing per press.

#include "hotkeys_internal.h"

#include <windows.h>

namespace acc::hotkeys {

namespace {

bool g_inited = false;

}  // namespace

void InitDefaults() {
    if (g_inited) return;
    g_inited = true;

    auto bind = [](Action a, int vk, int altVk = 0,
                   uint32_t req = 0, uint32_t forbid = 0) {
        g_defaults[static_cast<int>(a)] = {vk, altVk, req, forbid};
    };

    // ----- World interaction -----
    bind(Action::InteractTarget,       VK_RETURN, 0, 0,         kModShift);
    bind(Action::InteractForceRadial,  VK_RETURN, 0, kModShift, 0);
    // A second, separately rebindable key for the radial. The engine gives
    // Shift+R no meaning of its own, and input_pipeline swallows its plain R
    // under any modifier, so it cannot fire twice.
    bind(Action::InteractForceRadialSecondary, 'R', 0, kModShift, 0);
    bind(Action::TargetKey1,           '1',       0, 0,         kModShift);
    bind(Action::TargetKey2,           '2',       0, 0,         kModShift);
    bind(Action::TargetKey3,           '3',       0, 0,         kModShift);
    bind(Action::PersonalKey1,         '4',       0, 0,         kModShift);
    bind(Action::PersonalKey2,         '5',       0, 0,         kModShift);
    bind(Action::PersonalKey3,         '6',       0, 0,         kModShift);
    bind(Action::PersonalKey4,         '7',       0, 0,         kModShift);
    bind(Action::PersonalKey5,         '8',       0, 0,         kModShift);
    bind(Action::PersonalKey6,         '9',       0, 0,         kModShift);
    bind(Action::ActionBarOpen1,       '4',       0, kModShift, 0);
    bind(Action::ActionBarOpen2,       '5',       0, kModShift, 0);
    bind(Action::ActionBarOpen3,       '6',       0, kModShift, 0);
    bind(Action::ActionBarOpen4,       '7',       0, kModShift, 0);
    bind(Action::ActionBarOpen5,       '8',       0, kModShift, 0);
    bind(Action::ActionBarOpen6,       '9',       0, kModShift, 0);
    bind(Action::TargetActionOpen1,    '1',       0, kModShift, 0);
    bind(Action::TargetActionOpen2,    '2',       0, kModShift, 0);
    bind(Action::TargetActionOpen3,    '3',       0, kModShift, 0);
    bind(Action::LevelUpOpen,          'L',       0, kModShift, 0);
    // Unbound: a power-user panel, rebindable in the configurator.
    bind(Action::ExamineOpen,          0,         0, 0,         kAnyMod);
    // H has no engine binding, so Shift+H reaches no engine screen. Bare H is
    // the leader's own status.
    bind(Action::CombatQueueOpen,      'H',       0, kModShift, 0);
    bind(Action::SelfStatusAnnounce,   'H',       0, 0,         kAnyMod);

    // ----- Submenu navigation -----
    bind(Action::NavUp,                VK_UP,     0, 0, 0);
    bind(Action::NavDown,              VK_DOWN,   0, 0, 0);
    bind(Action::NavLeft,              VK_LEFT,   0, 0, 0);
    bind(Action::NavRight,             VK_RIGHT,  0, 0, 0);
    // The engine drops Home and End before our manager hook (no stock key map
    // entry), so they are polled; menus.cpp's PollHomeEndKeys turns a press
    // into a manager input event.
    bind(Action::NavHome,              VK_HOME,   0, 0, 0);
    bind(Action::NavEnd,               VK_END,    0, 0, 0);
    bind(Action::SubmenuEsc,           VK_ESCAPE, 0, 0, 0);
    bind(Action::QueueClearAll,        VK_RETURN, 0, kModShift, 0);

    // ----- View toggles on Q / E -----
    // Container give mode, store buy/sell and the K2 crafting create/break-down
    // view; the handlers gate on the panel kind, and Q/E do nothing in those
    // panels for the engine.
    bind(Action::ContainerGiveMode,    'Q', 'E', 0, 0);
    bind(Action::StoreModeToggle,      'Q', 'E', 0, 0);
    bind(Action::CraftViewToggle,      'Q', 'E', 0, 0);

    // ----- In-world cycle -----
    // , and . are the same VK on QWERTY and QWERTZ. On the / or - key each
    // variant forbids the stronger modifiers; RouteFocus forbids AltGr because
    // Windows adds a phantom Ctrl to AltGr on QWERTZ.
    bind(Action::CycleItemPrev,        VK_OEM_COMMA,  0, 0,         kModShift | kModCtrl);
    bind(Action::CycleCategoryPrev,    VK_OEM_COMMA,  0, kModShift, 0);
    bind(Action::CycleItemNext,        VK_OEM_PERIOD, 0, 0,         kModShift | kModCtrl);
    bind(Action::CycleCategoryNext,    VK_OEM_PERIOD, 0, kModShift, 0);
    // Ctrl+, / Ctrl+. jump to the first / last item of the category.
    bind(Action::CycleItemFirst,       VK_OEM_COMMA,  0, kModCtrl,
                                       kModShift | kModAlt | kModAltGr);
    bind(Action::CycleItemLast,        VK_OEM_PERIOD, 0, kModCtrl,
                                       kModShift | kModAlt | kModAltGr);
    bind(Action::AnnounceFocus,        VK_OEM_2, VK_OEM_MINUS, 0,
                                       kModShift | kModCtrl | kModAlt);
    bind(Action::PathfindFocus,        VK_OEM_2, VK_OEM_MINUS, kModShift,
                                       kModCtrl | kModAlt);
    bind(Action::PathfindFocusForce,   VK_OEM_2, VK_OEM_MINUS, kModAlt,
                                       kModCtrl);
    bind(Action::RouteFocus,           VK_OEM_2, VK_OEM_MINUS, kModCtrl,
                                       kModAltGr);

    // ----- Orientation and party -----
    // AltGr alone; Shift+AltGr belongs to the mouse-look probe.
    bind(Action::AnnounceDegrees,      VK_RMENU, 0, 0, kModShift);
    bind(Action::PartyLeaderAnnounce,  VK_TAB,   0, 0, 0);
    // N has no engine binding. N faces the region's exits, Shift+N drops a map
    // marker, Ctrl+N gives the exploration overview, Ctrl+Shift+N walks to
    // the nearest unexplored region.
    bind(Action::CameraOrient,         'N', 0, 0,         kAnyMod);
    bind(Action::SaveMarkerAtCursor,   'N', 0, kModShift, kModCtrl | kModAlt | kModAltGr);
    bind(Action::ExploreOverview,      'N', 0, kModCtrl,  kModShift | kModAlt | kModAltGr);
    bind(Action::ExploreWalkUnexplored, 'N', 0, kModCtrl | kModShift, kModAlt | kModAltGr);
    // Ctrl+L walks to and opens the nearest container with loot. Not Alt+N:
    // Alt+letter leaves Windows' keyboard-menu mode on, which unacquired the
    // engine's keyboard for the loot panel at the end of the walk.
    bind(Action::ExploreWalkLoot,      'L', 0, kModCtrl,  kModShift | kModAlt | kModAltGr);
    // Ctrl+H: everyone's health.
    bind(Action::PartyStatusAnnounce,  'H', 0, kModCtrl,  kModShift | kModAlt | kModAltGr);

    // ----- Editbox modal (queried only while an editbox is armed) -----
    bind(Action::EditboxReReadUp,      VK_UP,     0, 0, 0);
    bind(Action::EditboxReReadDown,    VK_DOWN,   0, 0, 0);
    bind(Action::EditboxSubmit,        VK_RETURN, 0, 0, 0);
    bind(Action::EditboxCancel,        VK_ESCAPE, 0, 0, 0);

    // ----- Help -----
    // F1 lists all keys, Ctrl+F1 the current screen's. Both are polled, so they
    // work everywhere. The engine also uses F1's input index as its GUI
    // activate code, so menus_dispatch.cpp keeps a raw F1 from reaching a panel.
    bind(Action::HelpMenuOpen,         VK_F1, 0, 0,        kAnyMod);
    bind(Action::HelpContext,          VK_F1, 0, kModCtrl, kModShift | kModAlt | kModAltGr);

    // ----- Diagnostic probes -----
    // The bare F-keys forbid Ctrl so the Ctrl+F11 / Ctrl+F12 camera-distance
    // probes (probe_camera_distance.h) don't fire them too. F11 is unbound.
    bind(Action::ProbeAudioCycle,            VK_F10, 0, 0, kModCtrl);
    bind(Action::ProbeAudioFire,             0,      0, 0, 0);
    bind(Action::ProbeCameraDump,            VK_F12, 0, 0, kModCtrl);
    bind(Action::ProbeCameraDistDump,        VK_F12, 0, kModCtrl, 0);
    bind(Action::ProbeCameraDistClampToggle, VK_F11, 0, kModCtrl, 0);

    // ----- Pazaak (polled while the board is foreground) -----
    // The tick Consume()s the world and menu actions on the shared keys.
    bind(Action::PazaakStand,       'S',       0, 0,         kAnyMod);
    bind(Action::PazaakEndTurn,     'E',       0, 0,         kAnyMod);
    bind(Action::PazaakReviewHand,  'C',       0, 0,         kAnyMod);
    bind(Action::PazaakReviewTable, 'T',       0, 0,         kAnyMod);
    bind(Action::PazaakNextCard,    VK_TAB,    0, 0,         kModShift);
    bind(Action::PazaakPrevCard,    VK_TAB,    0, kModShift, 0);
    bind(Action::PazaakPlay,        VK_RETURN, 0, 0,         kModShift);
    bind(Action::PazaakOptLeft,     VK_LEFT,   0, 0,         0);
    bind(Action::PazaakOptRight,    VK_RIGHT,  0, 0,         0);
    bind(Action::PazaakCancel,      VK_ESCAPE, 0, 0,         0);
    // The opponent's card count, which a sighted player sees.
    bind(Action::PazaakOppHand,     'C',       0, kModShift, kModCtrl | kModAlt | kModAltGr);

    // ----- Turret (polled while the gunner minigame runs) -----
    bind(Action::TurretCyclePrev,   'Q',       0, 0,         kAnyMod);
    bind(Action::TurretCycleNext,   'E',       0, 0,         kAnyMod);

    // ----- Swoop race (polled while a race runs) -----
    // W shifts up in both games: on K2 the engine reads only steering and the
    // mouse there, and Space stays its jump; on K1 Space is the engine's own
    // shift and stays one.
    bind(Action::SwoopShift,        'W',       0, 0,         kAnyMod);

    // ----- Dialog (polled while a dialog is foreground) -----
    // R repeats the NPC line; a dialog has no world target for vanilla R.
    bind(Action::DialogRepeatLine,  'R',       0, 0,         kAnyMod);

    // ----- Clipboard -----
    // Ctrl+R copies the last spoken text. Not Alt: Alt+letter leaves Windows'
    // keyboard-menu mode on and the game eats input. AltGr is forbidden as it
    // raises Alt plus a phantom Ctrl on QWERTZ.
    bind(Action::CopyLastSpoken,    'R',       0, kModCtrl,  kModShift | kModAlt | kModAltGr);

    // The live table starts as the defaults; EnsureOverridesLoaded lays the
    // user's rebinds over it once the settings file is reachable.
    for (int i = 0; i < static_cast<int>(Action::COUNT); ++i) {
        g_bindings[i] = g_defaults[i];
    }
}

}  // namespace acc::hotkeys

// The factory key bindings, one row per Action.

#include "hotkeys_internal.h"

namespace acc::hotkeys {

namespace {

// ----- Default-binding initialiser ------------------------------------------
// Filled the first time anything in this TU runs. Guarded so callers don't
// have to remember to call an init step — every entry point in the header
// (BeginTick, Pressed, Get, ...) routes through here.
//
// Order MUST match the Action enum. Mismatched ordering is the kind of bug
// that only surfaces at runtime as "Shift+B reads as `B` was pressed", so
// keep the comments aligned with `Action` declaration order in hotkeys.h.
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
    // Shift+R is a SECOND, independently-rebindable key for the same
    // force-radial action (its own keybind-list entry, not a replacement for
    // Shift+Enter). R is the vanilla "default action" key; Shift+R has no
    // distinct engine meaning (case 0xef ignores the shift bit), and the
    // modifier-space reservation in input_pipeline already swallows the
    // engine's plain R under any modifier — so it can't double-fire.
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
    // Unbound by default: Examine is a dev/power-user surface. Rebindable
    // through the keybind configurator for anyone who wants the panel.
    bind(Action::ExamineOpen,          0,         0, 0,
                                       kModShift | kModCtrl | kModAlt | kModAltGr);
    // Shift+H — H is engine-unbound, so Shift+H does NOT leak to any engine
    // screen (unlike the former Shift+K, which the engine read as plain K =
    // Skills/Feats/Force Powers). Bare H below is SelfStatusAnnounce.
    bind(Action::CombatQueueOpen,      'H',       0, kModShift, 0);
    // Bare H — quick HP / effects / equipped-weapon readout for the
    // currently-controlled leader. Shift+H is CombatQueueOpen (the
    // action-queue submenu), so forbid every modifier here to keep the
    // two routes distinct.
    bind(Action::SelfStatusAnnounce,   'H',       0, 0,
                                       kModShift | kModCtrl | kModAlt | kModAltGr);

    // ----- Submenu navigation -----
    // No modifier gate — these are pure routing keys. The Shift+Enter
    // QueueClearAll variant lives separately below.
    bind(Action::NavUp,                VK_UP,     0, 0, 0);
    bind(Action::NavDown,              VK_DOWN,   0, 0, 0);
    bind(Action::NavLeft,              VK_LEFT,   0, 0, 0);
    bind(Action::NavRight,             VK_RIGHT,  0, 0, 0);
    // Home / End: the engine drops these scancodes before our manager hook
    // (no [Keymapping] Action maps to KEYBOARD_HOME(32) / KEYBOARD_END(33)
    // in stock kotor.ini, verified empirically — patch-20260522-102841.log
    // shows zero events for these codes). Win32 polling is the only path
    // that reaches us, same as the cycle keys. menus.cpp's PollHomeEndKeys
    // synthesises a manager HandleInputEvent on rising edge.
    bind(Action::NavHome,              VK_HOME,   0, 0, 0);
    bind(Action::NavEnd,               VK_END,    0, 0, 0);
    bind(Action::SubmenuEsc,           VK_ESCAPE, 0, 0, 0);
    bind(Action::QueueClearAll,        VK_RETURN, 0, kModShift, 0);

    // ----- Container give-mode -----
    // Q or E — either key toggles. Mirrors the engine's in-panel Q/E
    // semantics (panel/character cycle) into "cycle the view". Container
    // and Store aren't part of the engine's strip cycle, so Q/E in those
    // panels is engine-noop and free for us to repurpose.
    bind(Action::ContainerGiveMode,    'Q', 'E', 0, 0);

    // ----- Store mode toggle. Same keys as ContainerGiveMode — the two
    // handlers gate on the foreground panel kind, so only one fires per
    // press.
    bind(Action::StoreModeToggle,      'Q', 'E', 0, 0);

    // ----- K2 crafting view toggle. Third action on the same keys, same
    // rule: the handler gates on the foreground panel kind, so exactly one
    // of the three fires per press. Flips the workbench / lab-station
    // screen between the create list and the break-down list, which is what
    // its "View Inventory" button does — the button is filtered out of
    // the chain in exchange, so the screen reads the same way a store or a
    // container does.
    bind(Action::CraftViewToggle,      'Q', 'E', 0, 0);

    // ----- In-world cycle -----
    // Cycle navigate keys use VK_OEM_COMMA / VK_OEM_PERIOD — same VK on
    // both QWERTY and QWERTZ. Modifier-precedence at the "/-" key is
    // expressed by forbidding the higher-precedence mods so each variant
    // only fires when no stronger modifier is held: AnnounceFocus forbids
    // all three; PathfindFocus (Shift) forbids Alt+Ctrl; PathfindFocusForce
    // (Alt) forbids Ctrl; RouteFocus (Ctrl) forbids AltGr (otherwise
    // QWERTZ AltGr+- double-fires as route because Windows synthesises a
    // phantom Ctrl alongside RMENU).
    bind(Action::CycleItemPrev,        VK_OEM_COMMA,  0, 0,         kModShift | kModCtrl);
    bind(Action::CycleCategoryPrev,    VK_OEM_COMMA,  0, kModShift, 0);
    bind(Action::CycleItemNext,        VK_OEM_PERIOD, 0, 0,         kModShift | kModCtrl);
    bind(Action::CycleCategoryNext,    VK_OEM_PERIOD, 0, kModShift, 0);
    // Ctrl+, / Ctrl+. jump to the first / last item of the current category.
    // Forbid Shift/Alt/AltGr so they stay distinct from the item/category
    // steps above and so QWERTZ AltGr+,/. (phantom Ctrl) doesn't fire them.
    bind(Action::CycleItemFirst,       VK_OEM_COMMA,  0, kModCtrl,  kModShift | kModAlt | kModAltGr);
    bind(Action::CycleItemLast,        VK_OEM_PERIOD, 0, kModCtrl,  kModShift | kModAlt | kModAltGr);
    bind(Action::AnnounceFocus,        VK_OEM_2, VK_OEM_MINUS, 0,
                                       kModShift | kModCtrl | kModAlt);
    bind(Action::PathfindFocus,        VK_OEM_2, VK_OEM_MINUS, kModShift,
                                       kModCtrl | kModAlt);
    bind(Action::PathfindFocusForce,   VK_OEM_2, VK_OEM_MINUS, kModAlt,
                                       kModCtrl);
    bind(Action::RouteFocus,          VK_OEM_2, VK_OEM_MINUS, kModCtrl,
                                       kModAltGr);

    // ----- Orientation & party -----
    // AnnounceDegrees: AltGr alone, no Shift. The mouse-look diagnostic
    // probe shares the key with Shift held, so forbid shift here to keep
    // the two routes distinct.
    bind(Action::AnnounceDegrees,      VK_RMENU, 0, 0, kModShift);
    bind(Action::PartyLeaderAnnounce,  VK_TAB,   0, 0, 0);
    // N alone — camera-orient (face the region's exits: nearest first, then clockwise; cardinal
    // step when no exits are known). Shift forbidden so we
    // don't fight SaveMarkerAtCursor (Shift+N drops a map marker).
    bind(Action::CameraOrient,         'N', 0, 0,
                                       kModShift | kModCtrl | kModAlt | kModAltGr);

    // ----- Map saved markers -----
    // Shift+N drops a marker at the map cursor's current world position.
    // N has zero engine-vanilla binding (verified against the controls-
    // and-input.md "Default keyboard controls" survey), so picking N
    // avoids the engine intercepting the keypress. Forbid Ctrl/Alt/AltGr
    // so this stays distinct from any future modifier variants.
    bind(Action::SaveMarkerAtCursor,   'N', 0, kModShift,
                                       kModCtrl | kModAlt | kModAltGr);
    // Ctrl+N — exploration overview (exit_memory): regions explored, the
    // nearest unexplored one and the exit toward it, loot left. N has no
    // engine binding, and both N bindings above forbid Ctrl.
    bind(Action::ExploreOverview,      'N', 0, kModCtrl,
                                       kModShift | kModAlt | kModAltGr);
    // Ctrl+Shift+N — walk toward that nearest unexplored region; Ctrl+L (L for
    // loot) — walk to and open the nearest container with items
    // (exit_memory). Loot was on Alt+N first, and that is exactly the Alt +
    // letter trap documented at CopyLastSpoken below: Windows' keyboard-menu
    // mode left the engine's DirectInput keyboard unacquired, so the loot
    // panel that opened at the end of the walk ignored every key until an
    // alt-tab (2026-11-08, Anouk). Ctrl+L: L has no Ctrl binding, and
    // LevelUpOpen (Shift+L) is not reached without Shift.
    bind(Action::ExploreWalkUnexplored, 'N', 0, kModCtrl | kModShift,
                                       kModAlt | kModAltGr);
    bind(Action::ExploreWalkLoot,      'L', 0, kModCtrl,
                                       kModShift | kModAlt | kModAltGr);
    // Ctrl+H — everyone's health (party_status). Bare H is the leader's own
    // status and forbids Ctrl; Shift+H (queue) requires Shift.
    bind(Action::PartyStatusAnnounce,  'H', 0, kModCtrl,
                                       kModShift | kModAlt | kModAltGr);

    // ----- View mode -----
    bind(Action::ViewModeToggle,       'B', 0, 0,         kModShift);
    bind(Action::CameraStateProbe,     'B', 0, kModShift, 0);

    // ----- Editbox modal -----
    // Same VKs as the submenu nav keys above; routing happens at the
    // call site (menus_editbox queries only while edit-mode armed).
    bind(Action::EditboxReReadUp,      VK_UP,     0, 0, 0);
    bind(Action::EditboxReReadDown,    VK_DOWN,   0, 0, 0);
    bind(Action::EditboxSubmit,        VK_RETURN, 0, 0, 0);
    bind(Action::EditboxCancel,        VK_ESCAPE, 0, 0, 0);

    // ----- Help system -----
    // F1 toggles the global keybind list; Ctrl+F1 reads the current screen's
    // keys. Both are Win32-polled (help::PollWin32) so they fire in every
    // context — in-world, menus, dialog, map. F1 forbids every modifier so it
    // stays distinct from Ctrl+F1; Ctrl+F1 requires Ctrl and forbids the rest.
    // The engine reuses InputIndex 0x27 (KEYBOARD_F1) as its GUI "activate"
    // code, so the menu manager hook also suppresses a raw F1 reaching a panel
    // (see menus.cpp) to stop F1 doubling as Enter in menus.
    bind(Action::HelpMenuOpen,         VK_F1, 0, 0,
                                       kModShift | kModCtrl | kModAlt | kModAltGr);
    bind(Action::HelpContext,          VK_F1, 0, kModCtrl,
                                       kModShift | kModAlt | kModAltGr);

    // ----- Diagnostic probes -----
    // Plain F9-F12 forbid Ctrl so the Option-B distance probes below can
    // own Ctrl+F11 / Ctrl+F12 without double-firing the bare-F-key probes.
    bind(Action::ProbeAudioCycle,      VK_F10, 0, 0, kModCtrl);
    // F11 unbound — the camera-distance clamp toggle (Ctrl+F11) needs the
    // key free for sustained per-mode testing; the fixed-North audio probe
    // it formerly hosted was getting in the way. Rebind here when re-
    // investigating the listener-frame question.
    bind(Action::ProbeAudioFire,       0,      0, 0, 0);
    bind(Action::ProbeCameraDump,      VK_F12, 0, 0, kModCtrl);

    // Option-B camera-distance probes. Ctrl-modified to disambiguate from
    // the bare F-key probes above. See probe_camera_distance.h for the
    // engine surfaces and intent.
    bind(Action::ProbeCameraDistDump,        VK_F12, 0, kModCtrl, 0);
    bind(Action::ProbeCameraDistClampToggle, VK_F11, 0, kModCtrl, 0);

    // ----- Pazaak minigame -----
    // Letter keys forbid every modifier so they stay distinct from Shift/
    // Ctrl combos elsewhere; the pollers in pazaak.cpp gate on the board
    // being foreground. Tab / Enter / arrows / Esc reuse the standard VKs —
    // the pazaak tick Consume()s the in-world / menu actions that share them
    // so only one handler fires per press.
    bind(Action::PazaakStand,       'S',       0, 0,         kModShift | kModCtrl | kModAlt | kModAltGr);
    bind(Action::PazaakEndTurn,     'E',       0, 0,         kModShift | kModCtrl | kModAlt | kModAltGr);
    bind(Action::PazaakReviewHand,  'C',       0, 0,         kModShift | kModCtrl | kModAlt | kModAltGr);
    bind(Action::PazaakReviewTable, 'T',       0, 0,         kModShift | kModCtrl | kModAlt | kModAltGr);
    bind(Action::PazaakNextCard,    VK_TAB,    0, 0,         kModShift);
    bind(Action::PazaakPrevCard,    VK_TAB,    0, kModShift, 0);
    bind(Action::PazaakPlay,        VK_RETURN, 0, 0,         kModShift);
    bind(Action::PazaakOptLeft,     VK_LEFT,   0, 0,         0);
    bind(Action::PazaakOptRight,    VK_RIGHT,  0, 0,         0);
    bind(Action::PazaakCancel,      VK_ESCAPE, 0, 0,         0);
    // Shift+C — opponent's remaining hand count (public info: sighted players
    // see the face-down cards). Bare C is PazaakReviewHand, so require Shift.
    bind(Action::PazaakOppHand,     'C',       0, kModShift, kModCtrl | kModAlt | kModAltGr);

    // ----- Turret minigame -----
    // Q/E cycle the locked fighter; polled only while the gunner minigame is
    // active (minigame_turret.cpp). Forbid every modifier like the Pazaak letters.
    bind(Action::TurretCyclePrev,   'Q',       0, 0,         kModShift | kModCtrl | kModAlt | kModAltGr);
    bind(Action::TurretCycleNext,   'E',       0, 0,         kModShift | kModCtrl | kModAlt | kModAltGr);

    // ----- Swoop race (both games) -----
    // Bare W shifts up a gear. Polled only while a swoop race is active
    // (minigame_swoop_race.cpp). On KOTOR 2 W is otherwise dead in the race —
    // the engine polls only the lateral axis and the mouse there, and its only
    // keyboard-free shift input is the left mouse button; Space keeps the K2
    // engine's own meaning, the jump. On KOTOR 1 Space is the engine's shift
    // and stays one; W is the same shift so the key carries between the games.
    // Forbid every modifier.
    bind(Action::SwoopShift,        'W',       0, 0,         kModShift | kModCtrl | kModAlt | kModAltGr);

    // ----- Dialog -----
    // Bare R re-speaks the current NPC dialog line. Vanilla R is the in-world
    // "default action on current target", but a dialog screen has no selected
    // world target, so R is free here. Polled (Win32) only while a dialog panel
    // is foreground (dialog_speech.cpp), like the cycle keys. Forbid every
    // modifier so it stays distinct from any future Shift/Ctrl+R combo.
    bind(Action::DialogRepeatLine,  'R',       0, 0,         kModShift | kModCtrl | kModAlt | kModAltGr);

    // ----- Clipboard -----
    // Ctrl+R copies whatever was spoken last (dialog line, journal entry, an NPC
    // name off the cycle keys) to the Windows clipboard. Sits on the same key as
    // the two other "repeat/act on what I just heard" binds — bare R re-speaks,
    // Shift+R is InteractForceRadialSecondary — so the R key reads as one family.
    //
    // Ctrl and not Alt: Alt is a Windows SYSTEM key, and Alt+letter left the
    // game window in the OS keyboard-menu mode that eats input until Alt is
    // tapped again. Swallowing SC_KEYMENU (focus_guard.cpp) demonstrably did
    // not clear it — two live rounds still blocked — so the key moved off Alt
    // entirely. Alt stays viable only for punctuation (PathfindFocusForce).
    //
    // AltGr is forbidden alongside Alt: on QWERTZ, AltGr raises kModAlt plus a
    // phantom Ctrl, so without this AltGr+R would copy.
    bind(Action::CopyLastSpoken,    'R',       0, kModCtrl,  kModShift | kModAlt | kModAltGr);

    // Seed the live table from the freshly-built defaults. User overrides (if
    // any) are layered on top later by EnsureOverridesLoaded once the settings
    // file is reachable (PatchDir known) — see BeginTick.
    for (int i = 0; i < static_cast<int>(Action::COUNT); ++i) {
        g_bindings[i] = g_defaults[i];
    }
}

}  // namespace acc::hotkeys

// The per-tick dispatcher (see core_tick.h) and its watchdog.

#include "core_tick_internal.h"

#include "announce_facing.h"
#include "audio_footstep_suppress.h"
#include "bridge.h"
#include "exit_memory.h"
#include "party_status.h"
#include "camera_announce.h"
#include "camera_orient.h"
#include "camera_spin_guard.h"
#include "combat.h"
#include "combat_diag.h"
#include "combat_queue.h"
#include "cycle_input.h"
#include "focus_guard.h"
#include "dialog_speech.h"
#include "diag_settings.h"
#include "discovery.h"
#include "endar_softlock.h"
#include "door_announce.h"
#include "engine_area.h"
#include "engine_player.h"
#include "engine_subscreen.h"
#include "examine_view.h"
#include "floor_puzzle.h"
#include "guidance_approach.h"
#include "guidance_autowalk.h"
#include "help.h"
#include "hotkeys.h"
#include "input_poll_router.h"
#include "engine_levelup.h"
#include "locked_recall.h"
#include "log.h"
#include "map_ui_cursor.h"
#include "map_user_markers.h"
#include "menus.h"
#include "menus_modsettings.h"
#include "menus_pazaakdeck.h"
#include "party_leader_announce.h"
#include "passive_narrate.h"
#include "minigame_pazaak.h"
#include "probe_camera_distance.h"
#include "probe_camera_state.h"
#include "engine_input.h"
#include "spatial_wall_surfaces.h"
#include "speech_clipboard.h"
#include "stealth_watch.h"
#include "weapon_set_watch.h"
#include "minigame_swoop_race.h"
#include "trap_watch.h"
#include "minigame_turret.h"
#include "transitions.h"
#include "engine_game.h"

// core_dllmain.cpp: the once-per-session stand-up (guards, speech, config
// snapshot, keymap, focus probe, bring-up nag).
void EnsureSessionBringup();

namespace acc::tick {

namespace {

// Tick watchdog. A multi-second freeze (sound cuts out, then resumes) means
// the game's main thread blocked, which leaves only a silent gap in the log.
// The watchdog turns that gap into one labelled line and logs nothing in
// steady state:
//
//   * Dispatch time. Each subsystem call is wrapped in PHASE(); a Dispatch
//     over kSlowDispatchMs logs its total and its slowest phase.
//   * The gap between ticks. A large gap with a small Dispatch total means
//     the stall was outside our Dispatch: engine code, an area load, or one
//     of our hooks firing between frames.

constexpr double kSlowDispatchMs = 200.0;
constexpr double kSlowGapMs      = 750.0;

LARGE_INTEGER g_qpcFreq    = {};
LARGE_INTEGER g_lastEnd    = {};
bool          g_haveLast   = false;

// The slowest phase of the current tick.
const char* g_worstName = nullptr;
double      g_worstMs   = 0.0;

// Timestamp of the tick currently being dispatched, in ms (see NowMs).
DWORD g_tickNowMs = 0;

// Time one Dispatch phase and fold it into the per-tick accumulators. The
// call runs exactly once, in place; only the surrounding timing is added.
#define PHASE(name, call)                          \
    do {                                           \
        LARGE_INTEGER _a = QpcNow();               \
        call;                                      \
        PhaseMark(name, QpcMs(_a, QpcNow()));      \
    } while (0)

inline LARGE_INTEGER QpcNow() {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t;
}

// QPC to ms, wrapping at 2^32 exactly like GetTickCount so every
// `now - then >= window` comparison stays valid unsigned arithmetic. Split
// into whole seconds and remainder: multiplying the raw counter by 1000
// would overflow int64 after ~29 years of uptime at a 10 MHz counter.
inline DWORD QpcToMs(const LARGE_INTEGER& t) {
    if (g_qpcFreq.QuadPart == 0) return GetTickCount();
    LONGLONG secs = t.QuadPart / g_qpcFreq.QuadPart;
    LONGLONG rem  = t.QuadPart % g_qpcFreq.QuadPart;
    return static_cast<DWORD>(secs * 1000 + (rem * 1000) / g_qpcFreq.QuadPart);
}

inline double QpcMs(const LARGE_INTEGER& a, const LARGE_INTEGER& b) {
    if (g_qpcFreq.QuadPart == 0) return 0.0;
    return static_cast<double>(b.QuadPart - a.QuadPart) * 1000.0 /
           static_cast<double>(g_qpcFreq.QuadPart);
}

inline void PhaseMark(const char* name, double ms) {
    if (ms > g_worstMs) { g_worstMs = ms; g_worstName = name; }
}

// Reset the accumulators and check the gap since the last tick. Returns the
// tick-start stamp.
LARGE_INTEGER WatchdogBeginTick() {
    if (g_qpcFreq.QuadPart == 0) QueryPerformanceFrequency(&g_qpcFreq);
    g_worstName = nullptr;
    g_worstMs   = 0.0;

    LARGE_INTEGER now = QpcNow();
    // Published before anything in the fan-out runs, so the whole tick sees
    // one "now".
    g_tickNowMs = QpcToMs(now);
    if (g_haveLast) {
        double gap = QpcMs(g_lastEnd, now);
        if (gap >= kSlowGapMs) {
            acclog::Write("Watchdog",
                          "SLOW FRAME gap=%.0fms since previous tick "
                          "(stall outside our Dispatch — engine load or a "
                          "between-frame hook; an area load can also show here)",
                          gap);
        }
    }
    return now;
}

void WatchdogEndTick(const LARGE_INTEGER& tickStart) {
    LARGE_INTEGER now = QpcNow();
    double total = QpcMs(tickStart, now);
    if (total >= kSlowDispatchMs) {
        acclog::Write("Watchdog",
                      "SLOW TICK total=%.0fms — slowest phase '%s' %.0fms",
                      total, g_worstName ? g_worstName : "?", g_worstMs);
    }
    g_lastEnd  = now;
    g_haveLast = true;
}

}  // namespace

DWORD NowMs() {
    // Before the first tick (bring-up, or a hook that fires ahead of any
    // Update) nothing is published yet; 0 would read as "a very long time
    // ago".
    return g_tickNowMs != 0 ? g_tickNowMs : GetTickCount();
}

void Dispatch() {
    LARGE_INTEGER tickStart = WatchdogBeginTick();

    // The phases gated on k1 exist only on KOTOR 1 or are not ported.
    const bool k1 = acc::game::IsKotor1();

    // KOTOR 1 stands the session up from OnRulesInit. KOTOR 2 has no such
    // hook, and the first tick is the earliest point past the loader lock
    // (this does file I/O and starts two worker threads) that is sure to be
    // reached. Idempotent; it must run before the reacquire drains below,
    // since the focus probe it starts produces their requests.
    if (!k1) EnsureSessionBringup();

    // Hotkey state for the whole tick; EndTick at the bottom keeps it for the
    // next tick's rising-edge checks.
    acc::hotkeys::BeginTick();

    // A focus regain or render-window recreation can leave the engine's
    // DirectInput keyboard unacquired. Reacquire on a clean tick, before any
    // handler samples the keyboard (see engine_input.h).
    acc::engine::DrainPendingReacquire();
    RetryColdStartReacquire();
    WatchDeadKeyboard();

    // Whether the engine still has a keyboard device at all: SetActive skips
    // a NULL device silently, so a released keyboard looks like a recovery
    // that keeps failing. Logs the device pointers on change and speaks once
    // when the keyboard goes.
    acc::engine::LogDirectInputStateIfChanged();

    // Scripted openings (the Endar Spire, Peragus) hold the global fade at
    // alpha 1.0 while the player has control, which gates MainLoop's
    // DoPassiveSelection off and freezes Q/E targeting and passive narration.
    // This drives it while the fade is the only blocker; see engine_area.cpp.
    acc::engine::MaybeDrivePassiveSelection();

    // The "Steam Big Picture is eating your keypresses" warning the
    // focus-probe thread queued.
    acc::focus_guard::DrainInputBlockedWarning();

    // Drop stale panel pointers before any handler touches them.
    PHASE("menus.ValidatePanels", acc::menus::ValidatePanels());

    // Before the help, menu and world pollers so the board can Consume() the
    // shared keys (Tab, Enter, arrows, Esc) first.
    PHASE("pazaak", acc::pazaak::Tick());

    // F1 / Ctrl+F1. Before the other pollers so an open list claims the nav
    // keys; after pazaak so the minigame keeps its own.
    PHASE("help", acc::help::PollWin32());

    // Ctrl+R copies the last spoken text.
    PHASE("speech_clipboard", acc::speech_clipboard::PollWin32());

    // Focus, contents and listbox change detection, give-mode poll.
    PHASE("menus.TickMonitors", acc::menus::TickMonitors());

    // Home/End in menus: the engine drops the scancodes, so we dispatch them.
    PHASE("menus.PollHomeEnd", acc::menus::PollHomeEndKeys());

    // Shift+N drops a map marker.
    PHASE("map_user_markers", acc::map_user_markers::PollWin32());

    if (k1) {
        // F12 / Ctrl+F12 camera diagnostics; their camera offsets are
        // unported.
        PHASE("probe_camera_state", acc::probe_camera_state::PollWin32());
        PHASE("probe_camera_distance", acc::probe_camera_distance::Tick());
    }

    // Cycle keys (`,` `.` `-`) and AltGr: the engine drops unbound scancodes.
    PHASE("cycle_input", acc::cycle_input::PollWin32());
    PHASE("announce_facing", acc::announce_facing::PollWin32());

    // Restore player input once the handed-off engine action drains from
    // the queue (or a ceiling backstop fires).
    PHASE("engine.inputRestore", acc::engine::TickPlayerInputRestore());

    // The walk-to-act approach tracker, armed by the autowalk and by Enter
    // (loot, talk, door): disarms quietly on arrival, says "way blocked" on
    // a walkmesh stall.
    PHASE("guidance.approach", acc::guidance::TickApproach());

    // Movement-key cancel, after TickApproach: when a dialog, loot or
    // cutscene took over this tick the tracker has already disarmed, so the
    // cancel can't clobber the scene's freshly queued move.
    PHASE("guidance.cancel", acc::guidance::PollMovementKeysCancel());

    // Logs the player's action-queue depth when it changes.
    PHASE("engine.actionQueueDiag", acc::engine::TickActionQueueDiag());

    // Deferred Q/E reannounce.
    PHASE("passive_narrate", acc::passive_narrate::Tick());
    PHASE("party_leader_announce", acc::party_leader_announce::Tick());

    // Order matters from here: camera_announce derives the camera yaw;
    // camera_orient reads it in the same frame for closed-loop arrival;
    // wall_surfaces rebuilds the wall cache on an area change; transitions
    // builds room_topology from that cache, so after wall_surfaces or the
    // first tick in a new area uses the old area's walls.
    PHASE("camera_announce", acc::camera_announce::Tick());
    // After camera_announce so its same-sector dedup sees this frame's
    // last-spoken direction.
    PHASE("door_announce", acc::door_announce::Tick());
    PHASE("locked_recall", acc::locked_recall::Tick());
    // The spin guard works around the edge-turn driver; keep the two together.
    PHASE("camera_orient", acc::camera_orient::Tick());
    PHASE("camera_spin_diag", acc::camera_spin_guard::Tick());
    PHASE("spatial.wall_surfaces", acc::spatial::wall_surfaces::Tick());

    // The minigames self-gate on CSWMiniGame.type: 1 swoop race, 2 turret.
    PHASE("swoop_race", acc::swoop_race::Tick());
    PHASE("turret_game", acc::turret_game::Tick());

    // Area and room transition announces.
    PHASE("transitions", acc::transitions::Tick());
    PHASE("discovery", acc::discovery::Tick());

    if (k1) {
        // Self-gated on their modules: the Endar Spire room-5 softlock guard
        // (which also flushes queued door hints) and the temple floor-plate
        // puzzle.
        PHASE("endar_softlock", acc::endar::Tick());
        PHASE("floor_puzzle", acc::floor_puzzle::Tick());
    }

    // Sighted-parity trap warnings off the engine's detected-by lists.
    PHASE("trap_watch", acc::trap_watch::Tick());
    PHASE("map_ui_cursor", acc::map_ui_cursor::Tick());

    // Stuck detection for OnPlayFootstep.
    PHASE("footstep_suppress", acc::audio::footstep_suppress::Tick());

    PHASE("combat.mode", acc::combat::TickCombatMode());
    PHASE("combat.log", acc::combat::TickCombatLog());
    PHASE("combat.absorb", acc::combat::TickCombatAbsorb());
    PHASE("combat.deflect", acc::combat::TickCombatDeflect());
    PHASE("combat.effects", acc::combat::TickCombatEffects());
    // One-shot audio-path report once the sound system is up.
    PHASE("audio_path", acc::diag::settings::LogAudioPathOnce());
    PHASE("party_leader.change", acc::party_leader_announce::TickLeaderChange());
    PHASE("combat.queue", acc::combat::queue::Tick());
    PHASE("combat_diag", acc::combat_diag::Tick());
    PHASE("combat.idle_notice", acc::combat::queue::TickIdleNotice());
    PHASE("examine_view", acc::examine_view::Tick());

    // Help overlay lifecycle: self-disarm on world teardown, list state.
    PHASE("help.tick", acc::help::Tick());

    // While stealthed: the closing distance to a hostile focus, every ~2 m.
    PHASE("stealth_watch", acc::stealth_watch::Tick());

    // Weapon-set swap announcement (KOTOR 2 only, gated inside).
    PHASE("weapon_set_watch", acc::weapon_set_watch::Tick());

    // Dialog screen and bark bubbles.
    PHASE("dialog_speech", acc::dialog_speech::Tick());

    // Enter (interact).
    PHASE("interact", acc::input_poll::PollHotkey());

    // After every input poll, so an add caused by this tick's dispatch has
    // already been counted.
    PHASE("queue_noop", acc::combat::queue::TickNoOpWatch());

    // Releases the level-up wizard's overlay pause once the panel closes (its
    // own buttons close it, so there is no close site to hook). After
    // PollHotkey so a wizard opened this frame is already live.
    PHASE("levelup_pause", acc::engine_levelup::TickLevelUpPause());

    // MessageBoxModal close cleanup: the engine's close path doesn't unpause
    // or unmute on its own.
    PHASE("engine.inputReassert", acc::engine::TickInputClassReassert());

    // Audio-glossary delayed playback.
    PHASE("modsettings", acc::menus::modsettings::Tick());

    // The pazaak deck builder's staged add/remove/play, before the pending-op
    // drain (a Play queues an Activate).
    PHASE("pazaakdeck", acc::menus::pazaakdeck::Tick());

    // Before the pending-op drain so a bridge action that queues menu work is
    // handled this same tick.
    PHASE("bridge", acc::bridge::Tick());

    // Visited regions: this area's set from the save, once settled.
    PHASE("exit_memory", acc::exit_memory::Tick());

    // "<name> can level up", once per member.
    PHASE("party_status", acc::party_status::Tick());

    // Queued actions last, so the monitors above saw consistent state.
    PHASE("menus.TickPendingOps", acc::menus::TickPendingOps());

    acc::hotkeys::EndTick();

    WatchdogEndTick(tickStart);
}

#undef PHASE

}  // namespace acc::tick

// CSWGuiManager::Update detour: @0x40ce76 on KOTOR 1 (argument = the manager,
// from EBP), @0x4113a9 on KOTOR 2 (argument = the frame pointer). Per-frame,
// post-input, so a safe site for deferred cursor moves. The argument is
// ignored, which lets one handler serve both hook shapes.
extern "C" void __cdecl OnUpdate(void* /*unused*/) {
    acc::tick::Dispatch();
}

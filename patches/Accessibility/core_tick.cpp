// Per-tick dispatcher — see core_tick.h.

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
#include "combat_query.h"
#include "combat_queue.h"
#include "cycle_input.h"
#include "focus_guard.h"
#include "dialog_speech.h"
#include "diag_settings.h"  // LogAudioPathOnce — one-shot audio-path report
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
#include "engine_levelup.h"  // TickLevelUpPause — release the wizard overlay pause
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
#include "view_mode.h"
#include "engine_game.h"

namespace acc::tick {

namespace {

// ---------------------------------------------------------------------------
// Tick watchdog
// ---------------------------------------------------------------------------
// Intermittent multi-second freezes (sound cuts out, then resumes) mean the
// game's main thread blocked — and a blocked thread leaves only a silent gap
// in the log. This watchdog turns that gap into one labelled line, then stays
// silent. It measures two independent things and logs ONLY past a threshold,
// so steady-state output is zero (a normal frame is well under 16 ms):
//
//   * Per-phase Dispatch time. Each subsystem call is wrapped in PHASE();
//     if the whole Dispatch overruns kSlowDispatchMs we log the total plus
//     the single slowest phase — naming the culprit when the stall is in
//     OUR code.
//   * Inter-tick wall gap. Time from the end of the previous Dispatch to the
//     start of this one. A large gap with a SMALL Dispatch total means the
//     stall was OUTSIDE our Dispatch (engine code, or one of our hooks firing
//     between frames) — the watchdog still catches it and says so.
//
// A genuine freeze is a single blocked tick, so it yields a single line, not
// a flood. Area loads can also trip the gap check — that is expected and
// informative, not spam (loads are rare and the line says "gap").
constexpr double kSlowDispatchMs = 200.0;   // our per-tick work over this → log
constexpr double kSlowGapMs      = 750.0;   // wall gap between ticks over this → log

LARGE_INTEGER g_qpcFreq    = {};
LARGE_INTEGER g_lastEnd    = {};
bool          g_haveLast   = false;

// Per-tick worst-phase accumulator (reset each Dispatch).
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

// QPC → ms, wrapping at 2^32 exactly like GetTickCount so every existing
// `now - then >= window` comparison stays valid unsigned arithmetic. Split
// into whole seconds + remainder rather than multiplying the raw counter by
// 1000, which would overflow int64 after ~29 years of uptime at a 10 MHz
// counter.
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

// Reset accumulators + check the inter-tick gap. Returns the tick-start stamp.
LARGE_INTEGER WatchdogBeginTick() {
    if (g_qpcFreq.QuadPart == 0) QueryPerformanceFrequency(&g_qpcFreq);
    g_worstName = nullptr;
    g_worstMs   = 0.0;

    LARGE_INTEGER now = QpcNow();
    // Publish this tick's timestamp before anything in the fan-out runs, so
    // every subsystem that asks sees the same "now" for the whole tick.
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

// Log the total if our Dispatch overran, naming the slowest phase.
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
    // Update) there is nothing published yet — fall back to the coarse clock
    // rather than handing out 0, which would read as "a very long time ago".
    return g_tickNowMs != 0 ? g_tickNowMs : GetTickCount();
}

void Dispatch() {
    LARGE_INTEGER tickStart = WatchdogBeginTick();

    // KOTOR 2 (Batch 1, GUI spine) runs the MENU-SYSTEM phases only: panel
    // validation, help overlay, the menu monitors (whose TickGeneralMonitors
    // is the sole consumer of the pending-announce slot), Home/End synthesis,
    // mod settings, the updater, and the pending-op drain. Every other phase
    // reads engine state its batch has not ported (world, combat, camera,
    // minigames, dialog, in-game panels) and is gated to KOTOR 1 below —
    // handler_chain_audit.py counts 300+ unresolved-constant reads across
    // those subsystems. Later batches clear their own phases from this gate.
    const bool k1 = acc::game::IsKotor1();

    // KOTOR 2 session bring-up. KOTOR 1 stands the session up from OnRulesInit;
    // KOTOR 2 has no rules-init hook, so the first engine tick is the earliest
    // point that is both past the loader lock (this does file I/O and starts
    // two worker threads) and guaranteed to be reached — unlike any menu hook,
    // which depends on a panel existing. Idempotent, and it must run before the
    // reacquire drains below: the focus probe it starts is what produces the
    // requests they drain.
    if (!k1) EnsureSessionBringup();

    // Snapshot hotkey state for the whole tick — EndTick at the bottom
    // shifts now→last for next tick's rising-edge math.
    acc::hotkeys::BeginTick();

    // If a focus regain / render-window recreation flagged a DirectInput
    // reacquire (windowed-mode external focus theft kills engine input
    // otherwise — see engine_input.h RequestInputReacquire), do it now on a
    // clean tick, before any handler samples keyboard state.
    acc::engine::DrainPendingReacquire();

    // Cold-start safety net: the one-shot reacquire at MainMenu first-sight
    // doesn't always take (the engine can drop it on a Render Window
    // recreation moments later), leaving the keyboard dead for tens of
    // seconds. Re-drive the edge until the pump is provably live, then stop.
    RetryColdStartReacquire();

    // Mid-session counterpart: the cold-start net latches off once the pump has
    // ever been live, and the engine's keyboard can die again on any later
    // focus regain. Fires only on the dead-keyboard fingerprint — see above.
    WatchDeadKeyboard();

    // What neither of the two above can see: whether the engine still HAS a
    // keyboard device to acquire. Both drive SetActive edges, and SetActive
    // skips a NULL device silently, so a released keyboard looks exactly like a
    // recovery that keeps not working. Reports the device pointers on change,
    // and speaks once if the keyboard is the one that went. Cheap: three
    // pointer hops and a compare, and it logs nothing while the state holds.
    acc::engine::LogDirectInputStateIfChanged();

    // The Endar Spire opening holds the global fade at alpha=1.0 (engine
    // re-asserts it every frame) after the fade finishes, while the player still
    // has world control. That gates MainLoop's DoPassiveSelection off, freezing
    // the Q/E candidate halo and passive narration (root cause of the second-
    // door "Q/E can't find the door in front of me"). Since the fade can't be
    // cleared, drive DoPassiveSelection ourselves while that's the only blocker.
    // No-op in normal play. See engine_area.cpp. Both games since the K2
    // systems pass (2026-08-02): the whole gate chain is witnessed in the K2
    // MainLoop twin — K2's scripted holds (Peragus opening) pin the fade the
    // same way the Endar Spire does.
    acc::engine::MaybeDrivePassiveSelection();

    // Speak the "Steam Big Picture is eating your keypresses" warning if the
    // focus-probe poll thread queued one (windowed-mode focus theft — the
    // user's keys never reach the engine, so menus look dead). Throttled at
    // the poll thread; this just drains + speaks on a safe main-thread tick.
    acc::focus_guard::DrainInputBlockedWarning();

    // Defensive — drop stale panel pointers before any handler touches them.
    PHASE("menus.ValidatePanels", acc::menus::ValidatePanels());

    // Pazaak minigame board — runs ahead of TickMonitors and the in-world /
    // menu pollers so it can Consume() the shared keys (Tab / Enter / arrows /
    // Esc) on its own tick before those pollers sample them. Both games since
    // the minigame port (2026-08-15): KOTOR 2's board is the same
    // CSWGuiPazaakGame driving the same CSWPazaak model, with a wider card
    // struct and five extra side-deck cards — see minigame_pazaak.cpp. Self-
    // gated on acquiring a real board panel.
    PHASE("pazaak", acc::pazaak::Tick());

    // Help system — F1 toggles the global keybind list, Ctrl+F1 reads the
    // current screen's keys. Runs ahead of the menu/cycle/interact pollers so
    // that while the list is open it claims the nav edges (Up/Down/Home/End/
    // Enter/Esc) before those consumers sample them. After pazaak so the
    // minigame keeps its own keys.
    PHASE("help", acc::help::PollWin32());

    // Ctrl+R copies the last spoken text to the clipboard. Position in the
    // phase order doesn't matter — Ctrl+R is bound to nothing else, so there is no
    // shared edge to arbitrate — but it belongs with the other always-on mod
    // services rather than with the state-gated world/menu pollers.
    PHASE("speech_clipboard", acc::speech_clipboard::PollWin32());

    // Menu monitors (focus/contents/listbox change detection, give-mode poll).
    PHASE("menus.TickMonitors", acc::menus::TickMonitors());

    // Home/End in menus — engine drops the scancodes; we synth-dispatch.
    PHASE("menus.PollHomeEnd", acc::menus::PollHomeEndKeys());

    // Shift+N drops a map marker; in-world Shift+N stays silent. Both games
    // since the map batch: the whole pin-creation chain (operator new, pin
    // ctor, note assign, AddMapPin) and the CSWCMapPin layout are witnessed
    // against K2's own script pin-creator.
    PHASE("map_user_markers", acc::map_user_markers::PollWin32());

    if (k1) {
        // Camera diagnostic probes (F12 camera state, Ctrl+F12 camera
        // distance). Stay KOTOR 1: pure dev diagnostics on 6 unresolved
        // camera/behavior vtable offsets — port when a K2 camera
        // investigation actually needs them, not before.
        PHASE("probe_camera_state", acc::probe_camera_state::PollWin32());
        PHASE("probe_camera_distance", acc::probe_camera_distance::Tick());
    }

    // Both games since the K2 systems pass (2026-08-02): view_mode.cpp's
    // whole closure is resolved — its one Todo (CClientOptions bitfield)
    // was witnessed off the K2 exe (chain and mouse-look identical; the
    // autopause field/bit MOVED and is Pick'd). Map-pin offsets are Same/
    // Pick'd since the map batch, so pins in the narrated-target slot read
    // correctly on both games.
    PHASE("view_mode.poll", acc::view_mode::PollWin32());

    // ----- Batch 3c (KOTOR 2 port): interaction -----
    // These run on BOTH games. The whole subsystem closure was resolved for
    // KOTOR 2 offline (92 constants, sole remaining Todo is the DESIGNED
    // kClientObjectServerObjectOffset whose consumers branch to the engine
    // resolver) — see docs/kotor2-port.md Batch 3c.

    // Cycle keys (`,`/`.`/`-`) and AltGr — engine drops unbound scancodes.
    PHASE("cycle_input", acc::cycle_input::PollWin32());
    PHASE("announce_facing", acc::announce_facing::PollWin32());

    // Restore player input once the handed-off engine action drains from
    // the queue (or a ceiling backstop fires).
    PHASE("engine.inputRestore", acc::engine::TickPlayerInputRestore());

    // Unified walk-to-act approach tracker — both the Shift+- autowalk and
    // the Enter-interact (loot/talk/door) dispatches arm it; it disarms
    // quietly on success and announces "way blocked" on a walkmesh-blocked
    // stall.
    PHASE("guidance.approach", acc::guidance::TickApproach());

    // W/S/A/D/C/Y movement-cancel. Runs AFTER TickApproach so that if a
    // dialog/loot/cutscene result surfaced this tick, the tracker has
    // already disarmed and the cancel sees nothing in flight — it can't
    // clobber a freshly-queued scripted move on the same tick the scene
    // takes over.
    PHASE("guidance.cancel", acc::guidance::PollMovementKeysCancel());

    // Diagnostic: log player action-queue depth changes (delta only).
    PHASE("engine.actionQueueDiag", acc::engine::TickActionQueueDiag());

    // ----- Batch 3 (KOTOR 2 port): world / area / transitions -----
    // These five run on BOTH games. Their whole read/call chains were
    // resolved for KOTOR 2 offline (camera pos/yaw Pick'd, LocalToWorld /
    // GetArea / client GetGameObject / GetNPCObject / GetObjectName all
    // Pick'd) — see docs/kotor2-port.md Batch 3.

    // Drain deferred Q/E reannounce.
    PHASE("passive_narrate", acc::passive_narrate::Tick());

    // Both games since the K2 systems pass (2026-08-02): its whole surface
    // (GetClientLeader / GetActiveLeaderName / GetPlayerPosition / hotkeys)
    // was already Pick'd and live on KOTOR 2 via other callers.
    PHASE("party_leader_announce", acc::party_leader_announce::Tick());

    // ----- ORDER LOAD-BEARING -----
    // camera_announce → camera_orient → spatial::wall_surfaces →
    // transitions → view_mode.
    //   camera_announce derives camera yaw from positions.
    //   camera_orient reads it for closed-loop arrival (same frame).
    //   spatial::wall_surfaces rebuilds the wall cache on area change.
    //   transitions builds room_topology that depends on the wall cache —
    //     must run AFTER wall_surfaces or the first tick of an area
    //     change uses stale walls from the previous area.
    //   view_mode reads camera yaw + walls + region/landmark caches.
    PHASE("camera_announce", acc::camera_announce::Tick());
    // Door-open facing readout — after camera_announce so its same-sector
    // dedup sees this frame's last-spoken direction. Cheap when idle.
    PHASE("door_announce", acc::door_announce::Tick());

    // Both games since the K2 systems pass (2026-08-02).
    // locked_recall: TLK lookup + narrated-target slot only, both ported.
    // camera_orient + spin guard ported TOGETHER (the guard is a workaround
    // for the active edge-turn driver — never split them): their surface is
    // camera pos/yaw (Pick'd, live via camera_announce), TurnScancode
    // (engine-constant-free — INI + DIK fallback) and GetMouseLook, whose
    // CClientOptions chain was witnessed on the K2 exe this pass.
    PHASE("locked_recall", acc::locked_recall::Tick());
    PHASE("camera_orient", acc::camera_orient::Tick());
    PHASE("camera_spin_diag", acc::camera_spin_guard::Tick());

    PHASE("spatial.wall_surfaces", acc::spatial::wall_surfaces::Tick());

    // Swoop race entry/exit cues, gear watch, race clock and the obstacle /
    // accelerator-pad spatial audio. Self-gated on CSWMiniGame.type == 1, so it
    // is inert outside a race. BOTH GAMES since the minigame port
    // (2026-08-15): KOTOR 2 keeps the whole CSWMiniGame / CSWTrackFollower /
    // CSWMiniGameObjectArray architecture — its three swoop areas (211TEL,
    // 371NAR, 510OND) load the same Type==1 minigame, and even stamp the same
    // MIN_TIME_* start globals the race clock reads. See docs/kotor2-port.md.
    PHASE("swoop_race", acc::swoop_race::Tick());

    // Turret / space-combat gunner minigame — shares CSWMiniGame with the
    // swoop race but reports type==2. Both games since 2026-10-24: KOTOR 2's
    // three turret areas (107PER, 421DXN, 505OND) run on the same module, with
    // the bolt ballistics and aim limits read per area and the turret-specific
    // offsets Pick()ed (see minigame_turret.cpp and
    // docs/llm-docs/k2-turret-model.md). Untested on KOTOR 2 at the time of
    // writing.
    PHASE("turret_game", acc::turret_game::Tick());

    // Area + room transition announces.
    PHASE("transitions", acc::transitions::Tick());

    // Both games since the K2 systems pass (2026-08-02): discovery's whole
    // closure was already Pick'd (ScriptVarTable 0x100->0x104, the four var
    // accessors, CExoString ctor/dtor — resolved in the Batch 3c address
    // rounds); only this gate was still standing.
    PHASE("discovery", acc::discovery::Tick());

    if (k1) {
        // Endar Spire room-5 softlock guard + plot-state diagnostic. Inert
        // outside END_M01AA (self-gated); flushes queued door hints every
        // frame.
        PHASE("endar_softlock", acc::endar::Tick());

        // Temple floor-plate puzzle assist — inert outside unk_m44ab.
        // Internally throttled to 150ms scans.
        PHASE("floor_puzzle", acc::floor_puzzle::Tick());
    }

    // Both games since the K2 systems pass (2026-08-02): the three per-kind
    // trap detected-by list offsets are witnessed in the K2 UpdateMineCheck
    // twin (see engine_area.h). Mirrors the engine's per-trap detected-by
    // lists for sighted-parity trap warnings; 250ms internal throttle.
    PHASE("trap_watch", acc::trap_watch::Tick());

    // Both games since the K2 systems pass (2026-08-02) — see the
    // view_mode.poll note above. Same slot as before (after floor_puzzle,
    // before map_ui_cursor) so cursor/listener ordering is unchanged.
    PHASE("view_mode", acc::view_mode::Tick());

    // Map UI cursor — gates on PanelKind::InGameMap. Both games since the
    // map batch: the transform block is Same (K2 SetMapData witness), the
    // hider/waypoint-list offsets and the fog + rotate entry points are
    // Pick'd from the K2 map ctor and fog-method decompiles.
    PHASE("map_ui_cursor", acc::map_ui_cursor::Tick());

    // Stuck-detection — feeds g_was_stuck for OnPlayFootstep. Both games
    // since the footstep port (2026-08-02): the whole chain was already
    // Pick'd (player position, area iteration, cached walls, combat mode);
    // only the K2 hook itself was missing (twin 0x00765E90, found after
    // the Batch-5 candidate 0x0077D390 was refuted as the class-selection
    // idle-animation driver).
    PHASE("footstep_suppress", acc::audio::footstep_suppress::Tick());

    // Combat — mode entry/exit, log narration, attack resolution, saves,
    // leader-change announce, examine panel monitor, queue submenu,
    // examine view, specials heartbeat. Both games since Batch 4: the
    // combat-round/action structs are witnessed on KOTOR 2 (action struct
    // byte-identical, round block at +0x150, creature→round at +0x10dc),
    // the msg listbox / pause / combat-mode accessors are Pick'd, and the
    // few still-unresolved diagnostics (effect icons, faction id, stealth)
    // decline under their own guards.
    PHASE("combat.mode", acc::combat::TickCombatMode());
    PHASE("combat.log", acc::combat::TickCombatLog());
    PHASE("combat.absorb", acc::combat::TickCombatAbsorb());
    PHASE("combat.deflect", acc::combat::TickCombatDeflect());
    PHASE("combat.effects", acc::combat::TickCombatEffects());
    // One-shot audio-path report (Miles 3D provider, dsoal, HRTF) once the
    // sound system is up; a no-op every tick after.
    PHASE("audio_path", acc::diag::settings::LogAudioPathOnce());
    PHASE("combat.leaderChange",
          acc::combat::query::TickLeaderChangeAutoAnnounce());
    PHASE("combat.queue", acc::combat::queue::Tick());
    PHASE("combat_diag", acc::combat_diag::Tick());
    PHASE("examine_view", acc::examine_view::Tick());

    // Help overlay lifecycle (self-disarm on world teardown, list state).
    // Engine-free apart from the in-world pause hold, whose engine calls are
    // guarded — and menus never take the hold. Runs on both games.
    PHASE("help.tick", acc::help::Tick());

    // Stealth distance readout — while the leader is stealthed and a
    // hostile creature is the narrated-target focus, speak the closing
    // distance every ~2 m. Inert otherwise (no stealth / no hostile
    // focus). Both games since the K2 systems pass (2026-08-02):
    // stealth_mode witnessed at K2 creature+0x511 (see stealth_watch.cpp).
    PHASE("stealth_watch", acc::stealth_watch::Tick());

    // Weapon-set swap announcement — KOTOR 2 only (gated inside; KOTOR 1
    // has no second weapon pair). Detects the cross-exchange of the four
    // weapon-slot handles (the engine stores no active-set flag), so the
    // SwitchWeaps key, the equip screen's swap button and scripted swaps
    // all announce what the leader now holds.
    PHASE("weapon_set_watch", acc::weapon_set_watch::Tick());

    // Dialog screen + bark bubble narration. Both games since Batch 3b —
    // its whole constant closure is resolved for KOTOR 2 (12/12; the reply
    // block re-derived past the message-ring collision) and its panels come
    // through the ported GUI spine.
    PHASE("dialog_speech", acc::dialog_speech::Tick());

    // Enter (interact) — engine click pipeline with localised pre-roll.
    // Both games since Batch 3c (picker chain + action-queue primitives
    // fully Pick'd; the KOTOR 2 in-world input hook carries the
    // modifier-space reservation this shares keys with).
    PHASE("interact", acc::input_poll::PollHotkey());

    // Verdict on any action fire that produced no queue add. After every input
    // poll above, so an add caused by this tick's dispatch has already been
    // counted; the watch itself only speaks once its window has expired.
    PHASE("queue_noop", acc::combat::queue::TickNoOpWatch());

    // Release the level-up wizard's overlay pause once the panel closes
    // (the wizard's own Accept/Back buttons close it, so there's no close
    // site to call EndOverlayPause). After PollHotkey so a wizard opened
    // this frame is already live when we check. Both games since the
    // level-up batch: the whole Shift+L chain (CanLevelUp, SetLevelUpMode,
    // both ShowLevelUpGUI variants, the wizard vtable) is Pick'd with
    // decompile witnesses.
    PHASE("levelup_pause", acc::engine_levelup::TickLevelUpPause());

    // MessageBoxModal close cleanup — engine's close path doesn't unpause
    // or unmute on its own. Both games since Batch 4/5: SetPauseState,
    // SetSoundMode (per-game signature) and the ExoSound global are all
    // Pick'd with byte-level witnesses.
    PHASE("engine.inputReassert", acc::engine::TickInputClassReassert());

    // Audio-glossary delayed-playback timer.
    PHASE("modsettings", acc::menus::modsettings::Tick());

    // Drain the Pazaak deck-builder's staged add/remove/play before the generic
    // pending-op drain (a Play queues an Activate that drains in TickPendingOps).
    // Both games since the minigame port (2026-08-15) — it no-ops unless its own
    // input handler staged something, which only happens on a real
    // CSWGuiPazaakStart.
    PHASE("pazaakdeck", acc::menus::pazaakdeck::Tick());

    // Local bridge: refresh the state snapshot and run queued requests from
    // the external client. Before the pending-op drain so a bridge action
    // that queues menu work is handled this same tick.
    PHASE("bridge", acc::bridge::Tick());

    // Visited regions: load this area's set from the save once settled.
    PHASE("exit_memory", acc::exit_memory::Tick());

    // "<name> can level up", once per member when it becomes true.
    PHASE("party_status", acc::party_status::Tick());

    // Drain queued actions LAST — monitors above must see consistent state.
    PHASE("menus.TickPendingOps", acc::menus::TickPendingOps());

    acc::hotkeys::EndTick();

    WatchdogEndTick(tickStart);
}

#undef PHASE

}  // namespace acc::tick

// CSWGuiManager::Update detour — @0x40ce76 on KOTOR 1 (argument = the manager,
// from EBP), @0x4113a9 on KOTOR 2 (argument = the frame pointer). Per-frame,
// post-input. Safe callback site for deferred cursor moves — input pipeline
// isn't mid-flight. The argument is ignored on both games, which is what makes
// one handler serve both hook shapes; Dispatch() itself decides per-game which
// phases run (Batch 1: KOTOR 2 gets the menu spine only).
extern "C" void __cdecl OnUpdate(void* /*unused*/) {
    acc::tick::Dispatch();
}

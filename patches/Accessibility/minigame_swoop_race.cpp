// Swoop race minigame accessibility — see minigame_swoop_race.h for design.
//
// Phase 1 lay-off (current): entry/exit announce + keybind cheat sheet,
// best-effort gear-shift announce derived from CSWTrackFollower.speed
// jumps, per-tick diagnostic snapshot (speed + tunnel offset). Latches
// the CSWMiniGame pointer on first detect because the area chain
// (GetCurrentArea → GetClientArea → mini_game) churns during the
// race-start transition and stops resolving the minigame even while
// the race is still running — verified live by patch-20260524-163552.log
// where ENTER fired at T+0 and a spurious EXIT fired at T+125 ms even
// though the actual race continued for 40 more seconds.

#include "minigame_swoop_race_internal.h"

#include "engine_area.h"      // GetClientArea + map-pin chain (back-pointer),
                              //   ReadGlobalNumber (MIN_RACE_GEAR / MIN_TIME_*)
#include "engine_game.h"      // IsKotor2 — gate ladder, jump watch, K2 keys
#include "engine_keymap.h"    // GameActionScancode — KOTOR 1's MGshoot bind
#include "hotkeys.h"          // Pressed(SwoopShift) — keyboard shift
#include "key_inject.h"       // Tap — KOTOR 1 shift through the MGshoot key
#include "log.h"
#include "speech.h"
#include "strings.h"          // Get(SwoopRaceStarted/Controls/Ended) +
                              //     FmtSwoopRaceGear
#include "engine_rebase.h"

namespace acc::swoop_race {

namespace {

// ============================================================================
// Engine struct offsets (from docs/llm-docs/re/swkotor.exe.h).
// ============================================================================
//
// CSWCArea.mini_game (verified line 8726, after rooms@+0x260):
//
// KOTOR 2 = +0x268. Witnessed as a WRITE, not inferred: the K2 area's
// minigame-load path @0x007A2FB0 allocates 0x11c bytes, runs the CSWMiniGame
// ctor @0x00839260 on it, and stores the result with `mov [ecx+0x268], edx`;
// the reader at 0x007AA940 loads the same slot. The +4 shift matches the map-pin
// array's (kClientAreaMapPinsOffset, 0x1c4 -> 0x1c8) — CSWCArea gained one dword
// low down and everything above moved together. Same delta again at the tail of
// the CSWMiniGame ctor, which takes the area's resref from area+0x110 on K2
// where KOTOR 1 takes it from +0x10c.
const size_t kClientAreaMiniGameOffset      = acc::off::Pick(0x264, 0x268);

// KOTOR 2 ONLY — the two per-slot script runners the client input handler
// (CClientExoAppInternal::HandleInputEvent twin @0x007b12c0) calls on the
// player bike. 173-byte twins of KOTOR 1's CSWMiniGameObject::OnHitFollower
// @0x0066c2d0: `if (param == 0) { GetScriptName(slot) -> valid? -> RunScript
// (name, mgo_index, 1) }`, with the slot index a literal:
//   @0x00838e10 -> slot 10 = OnAccelerate  (the gear ladder, onaccelerate.ncs)
//   @0x00838ec0 -> slot 11 = OnBrake       (the jump, onjump.ncs)
// The engine reaches them from event ids 0x16 (pad B) / 0x19 (left mouse
// button) for OnAccelerate and 0x1a (right mouse button) / 0xd9 (the MGshoot
// keymap row = Space) for OnBrake — events registered by K2's
// SetEventDescriptions twin @0x00785df0 (0x19 <- InputIndex 0 MOUSE_BUTTON0,
// 0x1a <- InputIndex 1 MOUSE_BUTTON1, class 1). KOTOR 1 sends 0x19 and 0xd9
// to CSWMiniPlayer::Fire instead, whose gun bank runs onfire.ncs — so Space
// shifts there and no keyboard key shifts on KOTOR 2 by default. The mod
// therefore re-routes: Space -> TryShift, W -> TryJump.
// __thiscall with one int (memory: the typedef must carry the int, or the
// callee reads caller-frame garbage).
const uintptr_t kAddrK2RunOnAccelerateScript = acc::addr::Kotor2Only(0x00838e10);
const uintptr_t kAddrK2RunOnBrakeScript      = acc::addr::Kotor2Only(0x00838ec0);
typedef void (__thiscall* PFN_RunFollowerSlotScript)(void* follower, int flag);

// KOTOR 1's shift input: [Keymapping] Action217 = keymap.2da row 17 MGshoot
// (event 0xd9), Space when the ini has no line for it. See TryShift.
constexpr int kK1MGShootActionId = 217;
constexpr int kDikSpace          = 0x39;

// ============================================================================
// SEH-guarded primitive reads. Same pattern as the rest of engine_*.
// ============================================================================

// Read CSWCArea.mini_game via the player-area chain. Source of truth at
// the moment of detection; we latch the result so churn in this chain
// during transitions doesn't drop us mid-race.
void* ReadMiniGameViaArea() {
    void* serverArea = acc::engine::GetCurrentArea();
    if (!serverArea) return nullptr;
    void* clientArea = acc::engine::GetClientArea(serverArea);
    return SafeReadPtr(clientArea, kClientAreaMiniGameOffset);
}

// Validate a latched CSWMiniGame pointer by re-reading its vtable and
// comparing against the value captured at latch time. Returns true iff
// the pointer is still readable and the vtable hasn't been overwritten
// (engine frees CSWMiniGame at race-end and the heap slot tends to get
// reused — vtable mismatch is a reliable "no longer the same struct"
// signal).
bool LatchedStillValid() {
    if (!g_state.latched_mini_game || !g_state.latched_vtable) return false;
    void* vt = SafeReadPtr(g_state.latched_mini_game, kMiniGameVtableOffset);
    return vt == g_state.latched_vtable;
}

// The obstacle, accelpad and side-wall cues (the per-tick MGO walk, the
// AsObstacle / AsEnemy downcasts, the LoopSource lifecycles) live in
// minigame_swoop_audio*.cpp; the race lifecycle below calls
// TickSpatialAudio() / ResetSpatialAudio().

// ============================================================================
// Speech.
//
// Combine opener + keybind cheat sheet into ONE interrupting utterance so
// the two halves can't preempt each other. Without this combine, the
// observed live behaviour was: opener cancelled by controls cancelled
// by the spurious EXIT 125 ms later, so the user heard nothing
// coherent.
// ============================================================================

void AnnounceEntry() {
    const char* opener   = acc::strings::Get(acc::strings::Id::SwoopRaceStarted);
    // KOTOR 2 rebinds the race: Space is the JUMP (OnBrake slot -> onjump.ncs)
    // and the gear shift moved to the OnAccelerate slot — see the string's
    // comment in strings.h for the input ids behind that.
    const char* controls = acc::strings::Get(
        acc::game::IsKotor2() ? acc::strings::Id::SwoopRaceControlsK2
                              : acc::strings::Id::SwoopRaceControls);
    if ((!opener || !*opener) && (!controls || !*controls)) return;

    char buf[512];
    std::snprintf(buf, sizeof(buf), "%s %s",
                  opener   ? opener   : "",
                  controls ? controls : "");
    acc::speech::Speak(buf, /*interrupt=*/true);
    acclog::Write("SwoopRace", "spoke entry: [%s]", buf);
}

// ============================================================================
// State transitions.
// ============================================================================

void HandleEnter(void* mg) {
    g_state.active              = true;
    g_state.latched_mini_game   = mg;
    g_state.latched_vtable      = SafeReadPtr(mg, kMiniGameVtableOffset);
    g_state.entered_at_ms       = GetTickCount64();
    g_state.last_diag_log_ms    = 0;
    g_state.ticks_since_lost    = 0;
    g_state.gear                = 0;
    g_state.last_max_speed      = 0.0f;
    g_state.gear_from_global    = false;
    g_state.shift_ready_announced = false;
    g_state.airborne            = false;
    g_state.jump_started_ms     = 0;
    g_state.jump_peak_z         = 0.0f;
    g_state.have_progress_mark  = false;
    g_state.stuck_hint_spoken   = false;
    g_state.last_hp             = -1;
    g_state.wrecked             = false;
    g_state.last_gear_change_ms = 0;
    g_state.have_start_ms       = false;
    g_state.race_start_ms       = 0;
    g_state.race_mph            = 2;
    g_state.race_max_speed      = 0.0f;
    g_state.race_peak_speed     = 0.0f;
    g_state.have_race_time      = false;
    g_state.race_time_seconds   = 0.0f;
    g_state.race_time_announced = false;
    // Defensive cleanup for obstacle + accelpad loops — any active
    // loop from a previous race must not survive into this one.
    ResetSpatialAudio();

    acclog::Write("SwoopRace",
                  "ENTER mini_game=%p vtable=%p",
                  mg, g_state.latched_vtable);

    // Speak FIRST, dump SECOND. The full byte dump fires a couple dozen
    // log writes which take real wall-time; doing them before
    // speaking risks the engine ticking again before we queue the
    // utterance, in which case TickGearWatch / EmitDiagSnapshot would
    // pile up on the same audio frame.
    AnnounceEntry();
    EmitEntrySummary(mg);
    if (!g_state.full_diagnostic_emitted) {
        EmitDiagnosticDump(mg);
        g_state.full_diagnostic_emitted = true;
    }
}

void HandleExit() {
    ULONGLONG dur = GetTickCount64() - g_state.entered_at_ms;
    acclog::Write("SwoopRace",
                  "EXIT after %llu ms (debounced %d ticks)",
                  dur, kExitDebounceTicks);
    // Stop every obstacle + accelpad loop — some may still be active
    // if the race ended while objects were in range.
    ResetSpatialAudio();
    g_state.active              = false;
    g_state.latched_mini_game   = nullptr;
    g_state.latched_vtable      = nullptr;
    g_state.ticks_since_lost    = 0;
    g_state.gear                = 0;
    g_state.last_max_speed      = 0.0f;
    g_state.gear_from_global    = false;
    g_state.shift_ready_announced = false;
    g_state.airborne            = false;
    // Fallback: if the terminal-stop detector already spoke the time (the
    // common path), this is a no-op; otherwise speak it now.
    SpeakRaceEndMessage();
    FlushRaceEndSpeech(/*force=*/true);   // a delayed time must not die with the module
}

// Run one of the K2 player bike's slot scripts through the engine's own
// runner, mirroring the guards the client input handler applies (a live
// swoop-type minigame with a player). SEH-guarded; false = nothing ran.
bool RunPlayerSlotScript(uintptr_t runner, const char* what) {
    if (!acc::game::IsKotor2() || !g_state.active) return false;
    void* mg = g_state.latched_mini_game;
    if (!mg || runner == 0) return false;
    if (SafeReadU32(mg, kMiniGameTypeOffset) != kMiniGameTypeSwoop) return false;
    void* player = SafeReadPtr(mg, kMiniGamePlayerOffset);
    if (!player) return false;
    __try {
        reinterpret_cast<PFN_RunFollowerSlotScript>(runner)(player, 0);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write("SwoopRace", "%s: runner faulted (player=%p)", what, player);
        return false;
    }
    acclog::Write("SwoopRace", "%s: ran (gear global=%d)", what,
                  acc::engine::ReadGlobalNumber(kGearGlobalName));
    return true;
}

}  // namespace

bool IsActive() { return g_state.active; }

bool TryShift() {
    if (acc::game::IsKotor2()) {
        return RunPlayerSlotScript(kAddrK2RunOnAccelerateScript, "shift (OnAccelerate)");
    }
    // KOTOR 1: the shift is CSWMiniPlayer::Fire (the gun bank runs
    // onfire.ncs, the gear ladder), which HandleInputEvent @0x00621210 calls
    // on event 0xd9 — the MGshoot keymap row, Action217, Space by default.
    // Pressed through the player's own bind rather than by calling Fire, so
    // the engine's guards on that case (module + area present, not paused)
    // stay the engine's. The press reaches the client input hook as 0xd9,
    // where TryJump declines on KOTOR 1 and the engine shifts.
    if (!g_state.active || !g_state.latched_mini_game) return false;
    int scan = acc::engine_keymap::GameActionScancode(kK1MGShootActionId);
    if (scan == 0) scan = kDikSpace;
    acc::key_inject::Tap(scan);
    acclog::Write("SwoopRace", "shift (MGshoot key, scan 0x%02x): injected "
                  "(gear global=%d)", scan,
                  acc::engine::ReadGlobalNumber(kGearGlobalName));
    return true;
}

bool TryJump() {
    return RunPlayerSlotScript(kAddrK2RunOnBrakeScript, "jump (OnBrake)");
}

void Tick() {
    // ---- Re-derive from area chain (ground truth when it works). ----------
    void* mgArea = ReadMiniGameViaArea();

    if (!g_state.active) {
        // Idle state. Fire ENTER only for an actual swoop race
        // (CSWMiniGame.type==1). The turret / space-combat gunner
        // minigame shares this exact struct (same vtable) but reports
        // type==2 and is handled by minigame_turret.cpp — entering here
        // would mis-announce it as a swoop race (and run the gear /
        // accel / wall heuristics, which are meaningless for a turret).
        // type is populated by the time the area chain exposes mini_game.
        // (Engine-confirmed: CSWMiniGame::Load only ever sets type to 1 or 2.
        // We previously read +0x84=axis_x and matched 0/3 by coincidence.)
        if (mgArea && SafeReadU32(mgArea, kMiniGameTypeOffset) == kMiniGameTypeSwoop) {
            HandleEnter(mgArea);
        }
        return;
    }

    // ---- We're active. Verify the latch is still alive. ------------------
    //
    // Two truth sources:
    //   1. Area chain still reports a mini_game (might be the same
    //      pointer, might be a fresh one for a new race).
    //   2. Latched pointer's vtable still reads the expected value.
    //
    // EXIT only when BOTH say "gone" continuously for kExitDebounceTicks.

    if (mgArea && mgArea != g_state.latched_mini_game) {
        // Area says minigame exists but at a different address. This
        // shouldn't happen mid-race, but if it does (engine swap, mod-
        // injected reset) re-latch onto the fresh struct so subsequent
        // reads track it.
        void* vt = SafeReadPtr(mgArea, kMiniGameVtableOffset);
        acclog::Write("SwoopRace",
                      "re-latch: old=%p new=%p vtable=%p",
                      g_state.latched_mini_game, mgArea, vt);
        g_state.latched_mini_game = mgArea;
        g_state.latched_vtable    = vt;
        g_state.ticks_since_lost  = 0;
    } else if (mgArea && mgArea == g_state.latched_mini_game) {
        // Area chain agrees with latch. Definitely alive.
        g_state.ticks_since_lost = 0;
    } else if (!mgArea && LatchedStillValid()) {
        // Area chain dropped to null (we lost GetCurrentArea visibility
        // — the race-start transition does this), but the latched
        // struct's vtable still reads as the same address it had at
        // ENTER, so the struct itself hasn't been freed. Stay locked.
        g_state.ticks_since_lost = 0;
    } else {
        // Both sources say gone. Hold off the announcement in case
        // it's a transient cross-tick race condition (engine briefly
        // overwriting / reallocating struct between area-side reads).
        ++g_state.ticks_since_lost;
        if (g_state.ticks_since_lost >= kExitDebounceTicks) {
            HandleExit();
            return;
        }
    }

    // ---- In-race observation (only when the latch is alive). -------------
    if (!g_state.latched_mini_game) return;

    // Keyboard shift: the bare W hotkey, on BOTH games so the muscle memory
    // carries between them (KOTOR 1's Space still shifts too). On KOTOR 2
    // Space / pad A (jump) and pad B (shift) arrive as events at the client
    // input hook (input_pipeline.cpp); W never does (the race polls it as an
    // axis, not an event), so it is polled here instead.
    if (acc::hotkeys::Pressed(acc::hotkeys::Action::SwoopShift)) {
        TryShift();
    }

    TickGearWatch(g_state.latched_mini_game);
    TickShiftReady(g_state.latched_mini_game);
    TickJumpWatch(g_state.latched_mini_game);
    TickHitPoints(g_state.latched_mini_game);   // log only; see DecideWreckAtFinish
    // TickRaceTimer first so have_start_ms is current this tick: it flips true
    // at the "Go!" launch (actual speed > 1 + fresh MIN_TIME stamp). Gate the
    // obstacle + accelpad proximity cues behind it so they stay silent through
    // the pre-race countdown and only begin once the race is actually underway.
    TickRaceTimer(g_state.latched_mini_game);
    if (g_state.have_start_ms) {
        TickSpatialAudio(g_state.latched_mini_game);
    }
    TickStuckHint(g_state.latched_mini_game);
    FlushRaceEndSpeech(/*force=*/false);
    EmitDiagSnapshot(g_state.latched_mini_game);
}

}  // namespace acc::swoop_race

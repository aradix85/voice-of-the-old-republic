// Internal to the minigame_swoop_race module (minigame_swoop_race*.cpp); no other file includes
// it. The public interface is minigame_swoop_race.h. Here: what the files share.

#pragma once

#include "minigame_swoop_race.h"

#include "minigame_aim.h"

#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "engine_offsets_select.h"
#include "minigame_swoop_audio.h"  // ParkedBar, the spatial audio ticks

namespace acc::swoop_race {

// SEH read primitives + minigame-object-array resolution are shared across
// the minigame TUs (see minigame_aim.h). Brought into unqualified scope so
// the dense reads below read as they did when each file had its own copy.
using acc::minigame::SafeReadPtr;
using acc::minigame::SafeReadU32;
using acc::minigame::SafeReadFloat;
using acc::minigame::SafeReadVector;
using acc::minigame::ResolveMgoArray;
using acc::minigame::CallAsCast;

// CSWMiniGame (line 9309). type/counts confirmed by name; obstacle data
// pointer at +0x44 + count at +0x48 confirmed from the patch-
// 20260524-163552.log first-fire byte dump (obstacles_ptr=0x1721AA08,
// count=22, the resref "m03mg" sits at the +0x0c CResRef as expected).
//
// KOTOR 2: CSWMiniGame is UNCHANGED through +0xec and merely grows a tail
// (0xf0 -> 0x11c: a unit Vector, the swoopupgrade.2da handle, and six per-level
// upgrade rows — K2's swoop-bike upgrades). Every field below is therefore
// Same(), each with its own witness in the K2 twins:
//   * ctor @0x00839260 — area at +0x28 (its only argument), player +0x24 nulled,
//     near/far clip 0.1f/100.0f at +0x68/+0x6c, view angle 65.0f at +0x70, and
//     five CExoArrayList triples constructed at +0x2c / +0x38 / +0x44 / +0x50 /
//     +0x5c, exactly the five KOTOR 1 zeroes inline (so the enemy id list stays
//     at +0x2c with its count at +0x30, and the obstacle list at +0x44 / +0x48);
//   * Load @0x0083db30 — branches on `[this+0x80] == 1` (swoop) vs `== 2`
//     (turret) to pick the MovementPerSec default, writes UseInertia/DoBumping
//     at +0x94/+0x95 and negates AxisX/AxisY at +0x84/+0x88;
//   * the resref lives in the CResHelper base at +0xc on both, and the streaming
//     sound source at +0x1c.
const size_t kMiniGameVtableOffset          = acc::off::Same(0x00);  // pointer
const size_t kMiniGameResrefOffset          = acc::off::Same(0x0c);  // CResRef[16]
const size_t kMiniGamePlayerOffset          = acc::off::Same(0x24);
const size_t kMiniGameEnemyCountOffset      = acc::off::Same(0x30);
const size_t kMiniGameObstacleDataOffset    = acc::off::Same(0x44);  // CSWMiniGameObject** (verified live)
const size_t kMiniGameObstacleCountOffset   = acc::off::Same(0x48);
const size_t kMiniGameTypeOffset            = acc::off::Same(0x80);  // 1=swoop, 2=turret
                                                        // (CSWMiniGame::Load sets
                                                        // type ONLY to 1 or 2; was
                                                        // mis-read at +0x84=axis_x,
                                                        // matching 0/3 by coincidence)
constexpr uint32_t kMiniGameTypeSwoop           = 1;

// CSWTrackFollower (line 15028) — base of CSWMiniPlayer. The byte-by-byte
// walk lines up with the swkotor.exe.h definition:
//   +0x00..0x5f  CSWMiniGameObject (60 bytes)
//   +0x60        mini_game ptr
//   +0x68..0x73  CExoArrayList models
//   +0x80        looping
//   +0x98        speed (float)
//
// KOTOR 2: unmoved. `speed` sits BELOW the script-CResRef array that grew from
// ten entries to thirteen (see minigame_aim.h), so it is one of the fields the
// +0x30 shift does not reach — its ctor twin @0x00833ba0 writes the same 100.0f
// default to dword index 0x26 = +0x98.
const size_t kFollowerSpeedOffset           = acc::off::Same(0x98);
// hp / max_hp sit just below speed (swkotor.exe.h: looping +0x80,
// sphere_radius +0x84, +0x88, hp +0x8c, max_hp +0x90, bump_damage +0x94,
// speed +0x98; SetHitPoints @0x0066c900 / SetMaxHitPoints @0x0066c950 write
// them). Below the script array on KOTOR 2 too — the first K2 player dump
// (patch-20260914-143533) shows 600 / 600 there, the .are's Hit_Points /
// Max_HPs for the Citadel bike.
const size_t kFollowerHpOffset              = acc::off::Same(0x8c);
const size_t kFollowerMaxHpOffset           = acc::off::Same(0x90);

// CSWMiniPlayer (line 15382, extends CSWTrackFollower size 0x1a4).
// Tunnel-frame offset (X=lane, Y=forward, Z=vertical) and the speed
// envelope.
//
// KOTOR 2 adds 0x30 to all three (follower base 0x1a4 -> 0x1d4 — K2's
// CSWMiniEnemy allocates exactly 0x1d4). min/max speed are direct witnesses:
// SetMinSpeed @0x00838240 writes +0x208, SetMaxSpeed @0x00838290 writes +0x20c,
// both reached from CSWMiniPlayer::Load's "Minimum_Speed" / "Maximum_Speed"
// reads. Full reasoning at kMiniPlayerOffsetVectorOffset in minigame_aim.h.
const size_t kMiniPlayerOffsetVectorOffset  = acc::off::Pick(0x1c4, 0x1f4);
const size_t kMiniPlayerMinSpeedOffset      = acc::off::Pick(0x1d8, 0x208);
const size_t kMiniPlayerMaxSpeedOffset      = acc::off::Pick(0x1dc, 0x20c);

// KOTOR 2 ONLY — the swoop bike's vertical (jump) velocity. It lives in the
// CSWMiniGame tail that KOTOR 1 does not have (K1's struct ends at 0xf0), not
// in the player. Witnessed in K2's CSWMiniPlayer::Control twin @0x00837960:
//   if (mini_game->type == 1) {            // swoop only
//       v = mini_game->+0x118;
//       offset_delta.z += v * dt;           // integrates into offset.z (+0x1fc)
//       mini_game->+0x118 = v - dt * 45.8f; // gravity, units/s²
//   }
// followed by KeepInTunnel, whose +Z bound the track scripts set to 10 (the
// onaccelerate gear-0 branch: SWMG_SetPlayerTunnelPos z=10). The only script
// writer is onjump.ncs — bound to the follower's OnBrake slot on all three K2
// tracks — which calls SWMG_SetJumpSpeed(30) unless the bike is already above
// z=2. 30 u/s against 45.8 u/s² gives a 1.3 s hop peaking at ~9.8 units, i.e.
// exactly under the tunnel ceiling. Read only on KOTOR 2 (Kotor2Only poisons
// the K1 side, and the consumers are IsKotor2()-gated as well).
const size_t kMiniGameJumpVelocityOffset    = acc::off::Kotor2Only(0x118);

// ----- Gear source: the tracks' own MIN_RACE_GEAR global -----
//
// Both games' swoop scripts keep the gear in the global-number table under
// MIN_RACE_GEAR (K1 heartbeat/onfire, K2 heartbeat/onaccelerate/enemyhitfollower,
// all disassembled): oncreate stamps -5, the countdown walks -4/-3/-2/-1, "Go"
// writes 0, every upshift writes 1..5, the finish writes 6 — and on KOTOR 2 an
// obstacle hit writes 0 again ("gear0" animation, engine idle), so the K2 gear
// can go DOWN mid-race. The max_speed-plateau heuristic below cannot see that
// (it only counts upward jumps, so a re-launch after a hit would read as gear 6,
// 7, ...), which is why the global is now the primary source on both games and
// the heuristic is the fallback for a modded track that does not maintain it.
// Values outside 0..5 are "no gear information" (countdown, finish, or a failed
// read — ReadGlobalNumber returns -1 on a fault, which lands in the same bucket
// as the -1 countdown step, deliberately).
constexpr int kGearGlobalMin = 0;
constexpr int kGearGlobalMax = 5;
constexpr const char* kGearGlobalName = "MIN_RACE_GEAR";

// Exit-debounce. After we lose the latched pointer, hold off announcing
// EXIT until the loss persists for this many consecutive ticks. The
// race-start transition flips the area chain in < 5 ticks, so 60 ticks
// (~2 s at 30 fps) is comfortably past that without making true exits
// feel laggy.
constexpr int   kExitDebounceTicks        = 60;
// The finish-line time is spoken this long after the finish so the engine's
// race-end sound has faded under it ("the end race sound is louder than the
// TTS", round 16). Inside the exit debounce (60 ticks ≈ 2 s); the exit path
// flushes it regardless. A wreck is spoken immediately.
constexpr ULONGLONG kRaceEndSpeakDelayMs  = 1200;

// ============================================================================
// Module state. Single-threaded under the engine OnUpdate tick.
// ============================================================================
struct State {
    // True iff we've announced ENTER and not yet announced EXIT.
    bool          active                  = false;
    // Latched CSWMiniGame pointer. Once we see a non-null mini_game via
    // the area chain we cache it here and verify per-tick by re-reading
    // its vtable; this is the only way to stay locked through the
    // race-start transition shuffle.
    void*         latched_mini_game       = nullptr;
    void*         latched_vtable          = nullptr;
    ULONGLONG     entered_at_ms           = 0;
    bool          full_diagnostic_emitted = false;
    ULONGLONG     last_diag_log_ms        = 0;
    // Exit debounce counter. Ticks since latched vtable became invalid;
    // resets when it re-validates. Announce EXIT only when this
    // crosses kExitDebounceTicks.
    int           ticks_since_lost        = 0;
    // Gear-tracking state. Reset on every fresh ENTER.
    //
    // Engine model (live-confirmed in patch-20260524-165828.log):
    // each manual shift WIDENS the speed envelope (gear 1 = [35..70],
    // gear 2 = [60..120], …). max_speed only moves UP and only on a
    // real shift event — within a gear, the bike accelerates from
    // min_speed toward max_speed without changing either bound. So
    // the true gear count is "number of distinct max_speed plateaus
    // observed so far", not "speed bucket within current band". A
    // jump of >= kGearShiftMaxDeltaM/s in max_speed counts as a new
    // gear.
    int           gear                    = 0;
    float         last_max_speed          = 0.0f;
    // True once MIN_RACE_GEAR has read a valid gear (0..5) this race. From
    // then on the global is the gear source and the plateau heuristic is
    // ignored, so the two never double-announce the same shift.
    bool          gear_from_global        = false;
    // "You can shift now" cue latch. True once we've fired the cue for the
    // CURRENT gear; re-armed by TickGearWatch on every detected shift, so the
    // cue fires at most once per gear. See ShiftGateSpeed / TickShiftReady.
    bool          shift_ready_announced   = false;
    ULONGLONG     last_gear_change_ms     = 0;   // see kShiftReadyHoldMs
    // ---- Hit points (CSWTrackFollower +0x8c / +0x90). K2's hit scripts take
    //      50 (obstacle) / 100 (mine) of the bike's 600 and the heartbeat ends
    //      the race at 0 — round 3 reached that at 72 s and the mod announced
    //      a finish time. Drops are logged only (the HUD has a damage gauge;
    //      per-hit speech was tried in round 4 and cut as noise); `wrecked`
    //      is decided when the race ends, see DecideWreckAtFinish.
    int           last_hp                 = -1;
    bool          wrecked                 = false;
    // ---- Jump state (KOTOR 2 only; K1 clamps offset.z to 0 so it never
    //      arms). Edge-detected on the tunnel offset's Z so a jump start and
    //      its landing are logged with the bike's speed, gear and world
    //      position — the data the vertical-obstacle cue will be tuned from.
    bool          airborne                = false;
    ULONGLONG     jump_started_ms         = 0;
    float         jump_peak_z             = 0.0f;
    // ---- Stuck detector (see TickStuckHint). Forward progress is measured
    //      on the camera's track-axis coordinate; the mark moves whenever the
    //      bike has gained kStuckProgressU since it was set.
    bool          have_progress_mark      = false;
    float         progress_mark_y         = 0.0f;
    ULONGLONG     progress_mark_ms        = 0;
    bool          stuck_hint_spoken       = false;
    ParkedBar     stuck_bar_spoken_for    = ParkedBar::None;   // re-arms when the situation changes
    // ---- Delayed race-end time (see kRaceEndSpeakDelayMs): the finish sound
    //      drowns speech spoken on the same frame, so the time waits.
    bool          race_end_pending        = false;
    ULONGLONG     race_end_due_ms         = 0;
    char          race_end_msg[160]       = {};
    // ---- Real race-timer state (see TickRaceTimer). ----
    // race_start_ms is the engine world-clock ms-of-day stamped at the "Go!"
    // signal, cached once from the MIN_TIME_* NWScript globals (constant for
    // the whole race). race_time_seconds is (world clock − start) frozen at
    // the finish-line crossing — the value announced on EXIT.
    bool          have_start_ms           = false;
    uint32_t      race_start_ms           = 0;
    uint32_t      race_mph                = 2;     // world calendar minutes/hour
    float         race_max_speed          = 0.0f;  // running top envelope speed
                                                   //   (envelope-finish "did we race" ref)
    float         race_peak_speed         = 0.0f;  // running top ACTUAL speed
                                                   //   (coast-fallback "did we race" ref —
                                                   //   must NOT use the envelope, which is
                                                   //   already 70 at launch and would let
                                                   //   the 1->10 launch ramp false-trigger)
    bool          have_race_time          = false;
    float         race_time_seconds       = 0.0f;
    // Idempotency latch: the race-end cue is spoken once — by whichever fires
    // first, the terminal-stop detector in TickRaceTimer (preferred: lands in
    // the clear air before the post-race heading narration) or HandleExit
    // (fallback for an abnormal end where the bike never decelerated).
    bool          race_time_announced     = false;
};

extern State g_state;

// Helpers shared across the files, by the file that defines them.

// minigame_swoop_race_timer.cpp
void TickRaceTimer(void* miniGame);
void SpeakRaceEndMessage();
void FlushRaceEndSpeech(bool force);

// minigame_swoop_race_cues.cpp
void TickGearWatch(void* miniGame);
void TickHitPoints(void* miniGame);
void TickStuckHint(void* miniGame);
void TickJumpWatch(void* miniGame);
void TickShiftReady(void* miniGame);

// minigame_swoop_race_diag.cpp
void EmitDiagnosticDump(void* miniGame);
void EmitEntrySummary(void* miniGame);
void EmitDiagSnapshot(void* miniGame);

}  // namespace acc::swoop_race

// Internal to the minigame_swoop_race module (minigame_swoop_race*.cpp); no other file includes
// it. The public interface is minigame_swoop_race.h. Here: what the files share.

#pragma once

#include "minigame_swoop_race.h"

#include "minigame_engine.h"

#include <windows.h>
#include <cstdint>
#include <cstdio>

#include "engine_offsets_select.h"
#include "minigame_swoop_audio.h"  // ParkedBar, the spatial audio ticks

namespace acc::swoop_race {

// The guarded reads the minigame files use (engine_reads.h).
using acc::engine::ReadPtrOrNull;
using acc::engine::ReadU32Or0;
using acc::engine::ReadFloatOr0;
using acc::engine::TryReadVector;

// CSWMiniGame.type of a swoop race.
constexpr uint32_t kMiniGameTypeSwoop = 1;

// CSWMiniPlayer (line 15382, extends CSWTrackFollower size 0x1a4).
// Tunnel-frame offset (X=lane, Y=forward, Z=vertical) and the speed
// envelope.
//
// KOTOR 2 adds 0x30 to all three (follower base 0x1a4 -> 0x1d4 — K2's
// CSWMiniEnemy allocates exactly 0x1d4). min/max speed are direct witnesses:
// SetMinSpeed @0x00838240 writes +0x208, SetMaxSpeed @0x00838290 writes +0x20c,
// both reached from CSWMiniPlayer::Load's "Minimum_Speed" / "Maximum_Speed"
// reads. Full reasoning at kMiniPlayerOffsetVectorOffset.
const size_t kMiniPlayerMinSpeedOffset      = acc::off::Pick(0x1d8, 0x208);
const size_t kMiniPlayerMaxSpeedOffset      = acc::off::Pick(0x1dc, 0x20c);

// ----- Gear source: the tracks' own MIN_RACE_GEAR global -----
//
// Both games' swoop scripts keep the gear in the global-number table under
// MIN_RACE_GEAR (K1 heartbeat/onfire, K2 heartbeat/onaccelerate/enemyhitfollower,
// all disassembled): oncreate stamps -5, the countdown walks -4/-3/-2/-1, "Go"
// writes 0, every upshift writes 1..5, the finish writes 6 — and on KOTOR 2 an
// obstacle hit writes 0 again ("gear0" animation, engine idle), so the K2 gear
// can go DOWN mid-race. The max_speed-plateau heuristic cannot see that (it
// only counts upward jumps, so a re-launch after a hit would read as gear 6,
// 7, ...), so the global is the primary source on both games and the
// heuristic is the fallback for a modded track that does not maintain it.
// Values outside 0..5 are "no gear information" (countdown, finish, or a failed
// read — ReadGlobalNumber returns -1 on a fault, which lands in the same bucket
// as the -1 countdown step, deliberately).
constexpr int kGearGlobalMin = 0;
constexpr int kGearGlobalMax = 5;
constexpr const char* kGearGlobalName = "MIN_RACE_GEAR";

// Exit debounce: after the latched pointer is lost, wait this many
// consecutive ticks before announcing the exit. The race-start transition
// flips the area chain in < 5 ticks, so 60 ticks (~2 s at 30 fps) is well
// past that without making a true exit feel laggy.
constexpr int   kExitDebounceTicks        = 60;
// The finish-line time is spoken this long after the finish, so the engine's
// race-end sound, which is louder than the speech, has faded under it.
// Inside the exit debounce; the exit path flushes it regardless. A wreck is
// spoken immediately.
constexpr ULONGLONG kRaceEndSpeakDelayMs  = 1200;

// The module state. Single-threaded under the engine OnUpdate tick; a fresh
// State{} on every entry and exit.
struct State {
    // True iff the entry was announced and the exit not yet.
    bool          active                  = false;
    // The CSWMiniGame pointer latched at entry, re-verified every tick by
    // its vtable; the only way to stay locked through the race-start
    // transition.
    void*         latched_mini_game       = nullptr;
    void*         latched_vtable          = nullptr;
    ULONGLONG     entered_at_ms           = 0;
    ULONGLONG     last_diag_log_ms        = 0;
    // Ticks since the latch stopped validating; the exit is announced when
    // this reaches kExitDebounceTicks.
    int           ticks_since_lost        = 0;
    // Gear tracking.
    //
    // Engine model for the plateau heuristic: each manual shift WIDENS the
    // speed envelope (gear 1 = [35..70], gear 2 = [60..120], ...). max_speed
    // only moves UP and only on a real shift — within a gear, the bike
    // accelerates from min_speed toward max_speed without changing either
    // bound. So the gear is the number of distinct max_speed plateaus seen so
    // far; a jump of at least kGearShiftMinJump counts as a new gear.
    int           gear                    = 0;
    float         last_max_speed          = 0.0f;
    // True once MIN_RACE_GEAR has read a valid gear (0..5) this race. From
    // then on the global is the gear source and the plateau heuristic is
    // ignored, so the two never double-announce the same shift.
    bool          gear_from_global        = false;
    // "You can shift now" cue latch: true once the cue fired for the CURRENT
    // gear; every detected shift re-arms it. See TickShiftReady.
    bool          shift_ready_announced   = false;
    ULONGLONG     last_gear_change_ms     = 0;   // see kShiftReadyHoldMs
    // ---- Hit points (CSWTrackFollower +0x8c / +0x90). K2's hit scripts take
    //      50 (obstacle) / 100 (mine) of the bike's 600 and the heartbeat ends
    //      the race at 0. Drops are logged only (per-hit speech was too
    //      noisy; the HUD's damage gauge is the sighted equivalent);
    //      `wrecked` is decided when the race ends, see DecideWreckAtFinish.
    int           last_hp                 = -1;
    bool          wrecked                 = false;
    // ---- Jump state (KOTOR 2 only; K1 clamps offset.z to 0 so it never
    //      arms). Edge-detected on the tunnel offset's Z so a jump start and
    //      its landing are logged with the bike's speed, gear and world
    //      position — the data a vertical-obstacle cue would be tuned from.
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
    // ---- Delayed race-end time (see kRaceEndSpeakDelayMs).
    bool          race_end_pending        = false;
    ULONGLONG     race_end_due_ms         = 0;
    char          race_end_msg[160]       = {};
    // ---- Race timer (see TickRaceTimer).
    // race_start_ms is the engine world-clock ms-of-day stamped at the "Go!"
    // signal, cached once from the MIN_TIME_* NWScript globals (constant for
    // the whole race). race_time_seconds is (world clock − start) frozen at
    // the finish-line crossing.
    bool          have_start_ms           = false;
    uint32_t      race_start_ms           = 0;
    uint32_t      race_mph                = 2;     // world calendar minutes/hour
    // Running top ENVELOPE speed: the "did we race" reference for the
    // envelope finish.
    float         race_max_speed          = 0.0f;
    // Running top ACTUAL speed: the reference for the coast fallback. Not
    // the envelope, which is already 70 at launch and would let the 1 -> 10
    // launch ramp trigger it.
    float         race_peak_speed         = 0.0f;
    bool          have_race_time          = false;
    float         race_time_seconds       = 0.0f;
    // The race-end cue is spoken once, by whichever fires first: the finish
    // detector in TickRaceTimer (preferred: clear air before the post-race
    // narration) or HandleExit (an abnormal end).
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
void EmitEntrySummary(void* miniGame);
void EmitDiagSnapshot(void* miniGame);

}  // namespace acc::swoop_race

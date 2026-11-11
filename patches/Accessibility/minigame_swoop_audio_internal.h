// Internal to the minigame_swoop_audio module (minigame_swoop_audio*.cpp); no other file includes
// it. The public interface is minigame_swoop_audio.h. Here: what the files share.

#pragma once

#include "minigame_swoop_audio.h"

#include <windows.h>
#include <cmath>
#include <cstdint>
#include <cstddef>

#include "audio_cues.h"       // NavCue + GetNavCueResref (centralised
                              //     resref vocabulary — Audio glossary
                              //     and live race fire the same samples)
#include "audio_loop.h"       // LoopSource — sustained warning per
                              //     obstacle / nearest-pad cue
#include "minigame_aim.h"     // shared offset read/write + magnetism (the
                              //     turret aim-assist port — lateral steering)
#include "engine_offsets_select.h"
#include "minigame_swoop_audio_tuning.h"

namespace acc::swoop_race {

// Shared SEH reads and minigame-object-array helpers (minigame_aim.h).
using acc::engine::ReadPtrOrNull;
using acc::engine::ReadU32Or0;
using acc::engine::ReadFloatOr0;
using acc::engine::TryReadVector;
using acc::minigame::ResolveMgoArray;
using acc::minigame::CallAsCast;
using acc::minigame::ReadFollowerPosition;
using acc::minigame::ReadFollowerModelGob;
using acc::minigame::ReadGobBoundingBox;
using acc::minigame::ReadGobPartBoundingBox;

// ============================================================================
// Module state. Single-threaded under the engine OnUpdate tick.
// ============================================================================
struct SpatialAudioState {
    // Per-slot looping cues. Sized to the global MGO array slot count
    // so we can key by slot without a parallel index map. An MGO slot
    // is either an obstacle or an enemy (the AsXxx vtable downcasts
    // return null for mismatches), never both, so the two arrays don't
    // overlap on any one slot. Auto-cleans at DLL unload via RAII.
    acc::audio::LoopSource obstacle_loops[kMgoArraySlotCount];
    // The panned steering-guide tone (acc_boost), aimed at the next gate or
    // the active detour point; centred = on the line.
    acc::audio::LoopSource accelpad_line_loop;
    // Bike world X last tick and the smoothed per-tick lateral velocity (the
    // magnet's brake and the wall cues).
    float prev_bike_x      = 0.0f;
    bool  have_prev_bike_x = false;
    float smoothed_vel     = 0.0f;
    // The pad slot targeted last tick; a change means a gate was crossed.
    int   prev_ahead_slot    = -1;
    // Panned-guide aligned confirmation: whether the guide tone was sounding
    // (off-pad) last tick, so we fire one centred "on track, stop steering" blip
    // on the steering→locked transition. Re-arms when you steer off again.
    bool      guide_was_active    = false;
    // Predictive overshoot/wall cue debounce: last time we fired the early
    // wall-impact sound, so it doesn't machine-gun while the bike sits pinned.
    ULONGLONG last_wall_cue_ms    = 0;
    acc::audio::LoopSource wall_loop;     // K2 wall band, see kWallBandU
    int       wall_loop_side       = 0;   // -1 left, +1 right, 0 silent
    int       detour_slot          = -1;  // mine the guide is detouring, -1 none
    // Lateral steering magnetism: throttle for the ~1/s SwoopAssist heartbeat
    // log (the assist writes offset.x every in-band tick; we only log once a
    // second so normal play isn't spammed).
    ULONGLONG last_magnet_log_ms  = 0;
    // Crossing-event scoring: last tick's targeted gate + its lateral miss and
    // forward gap, so when the target changes we can interpolate the EXACT
    // lateral miss at fwdGap=0. prev_ahead_slot above is the gate id.
    bool      have_prev_ahead    = false;
    float     prev_ahead_posX    = 0.0f;
    float     prev_ahead_posY    = 0.0f;
    float     prev_ahead_padErr  = 0.0f;
    float     prev_ahead_fwdGap  = 0.0f;
    // Diagnostic guards for the first-pass inventory dumps. One log
    // entry per race describing every obstacle / accelpad seen.
    bool obstacle_diag_emitted = false;
    bool accelpad_diag_emitted = false;
    // KOTOR 2 hazard geometry, resolved once per obstacle slot per race (the
    // boxes are static; the part walk touches mesh vertices, not per-tick
    // material). POD — this file is SEH-heavy.
    HazardInfo hazards[kMgoArraySlotCount];
    // KOTOR 2 mines: enemies (CSWMiniEnemy, model mgf_mine*) that sit still in
    // the lane and drop the bike to gear 0 on contact. The pad sweep finds them
    // (same pool as the pads) and hands them to the obstacle sweep as blocks.
    int    mine_count = 0;
    int    mine_slot[kMgoArraySlotCount];
    Vector mine_pos[kMgoArraySlotCount];
    float  mine_radius[kMgoArraySlotCount];   // sphere_radius, live (0 = read faulted)
    // Mine-pass log (see kMinePassWindowU): per mine slot, the closest 3-D
    // approach of the current pass and the hp when the pass began.
    bool   mine_pass_open[kMgoArraySlotCount];
    float  mine_pass_min_d3[kMgoArraySlotCount];
    float  mine_pass_dx[kMgoArraySlotCount];
    float  mine_pass_dy[kMgoArraySlotCount];
    float  mine_pass_dz[kMgoArraySlotCount];
    int    mine_pass_hp0[kMgoArraySlotCount];
    float  ground_z = 40.0f;   // bike world z last seen on the ground (mine reach)
    // KOTOR 2 bars in world space (the obstacle sweep rebuilds this every
    // tick from the resolved boxes; the pad sweep, which runs first, reads the
    // previous tick's copy — the geometry is static). The bar-aware guide
    // (kBarFreeClearU) and the pair-aware jump lead (kPairMinWindowSec) read it.
    // has_gap / gap_x / gap_side: the free-side strip (kPostContactU block).
    struct BarInfo { int slot; float x0, x1, y0, y1; bool has_gap; float gap_x; int gap_side; };
    int     bar_count = 0;
    BarInfo bars[kMgoArraySlotCount];
    int     bar_detour_slot      = -1;   // bar the guide is skirting, -1 none
    int     bar_air_cleared_slot = -1;   // bar released mid-flight (landing past it)
    bool    bar_gap_diag_emitted = false;
    ParkedBar parked_bar         = ParkedBar::None;   // see GetParkedBarSituation
    // Airborne state last tick (offset.z) — the touchdown edge re-arms one
    // "jump again" beep for a bar still ahead.
    bool   was_airborne = false;
    // Track-axis probe (see TrackAxisUsable). Decided once per race from the
    // bike's own travel; until it is, the geometry cues stay silent.
    int    axis_state       = kAxisUndecided;
    bool   have_axis_origin = false;
    Vector axis_origin      = {0.0f, 0.0f, 0.0f};
};

extern SpatialAudioState g_state;

// Linear volume byte for a source at 3D distance `dist` within `range`:
// full (kSwoopVolNearByte) at the listener, ramping to kSwoopVolFarByte
// at the range edge. Clamped to the endpoints outside [0, range].
inline int SwoopVolumeByte(float dist, float range) {
    if (range <= 0.0f) return kSwoopVolNearByte;
    float t = dist / range;                 // 0 near .. 1 far
    if (t < 0.0f) t = 0.0f; else if (t > 1.0f) t = 1.0f;
    int vb = static_cast<int>(
        kSwoopVolNearByte + t * (kSwoopVolFarByte - kSwoopVolNearByte) + 0.5f);
    if (vb < kSwoopVolFarByte)  vb = kSwoopVolFarByte;
    if (vb > kSwoopVolNearByte) vb = kSwoopVolNearByte;
    return vb;
}

// Helpers shared across the files, by the file that defines them.

// minigame_swoop_audio.cpp
const char* ReadAurObjectName(void* aurObject);

// ---- The obstacle sweep, shared by minigame_swoop_audio_obstacles.cpp and
// minigame_swoop_audio_bars.cpp.

// Obstacles within this 3D distance of the listener cue: about 1-1.3 s of
// warning at the top bike speed (~190 u/s in gear 3 on Taris); 30 m gave
// about 150 ms, too late to pick a lane.
constexpr float kObstacleCueRangeM = 300.0f;

// Forward is +Y (the bike runs from low Y to high, confirmed live). An
// obstacle up to this far behind the listener keeps cueing, so the pan
// finishes its sweep mid-pass.
constexpr float kObstacleForwardMargin = 10.0f;

// The bike, the listener and this tick's collections, shared by the steps
// of TickObstacleCues. POD apart from the initialisers: the swoop files are
// SEH-heavy by convention (see CLAUDE.md).
struct ObstacleTick {
    bool   k2           = false;
    Vector bikePos      = {0, 0, 0};
    bool   bikeOk       = false;
    float  bikeSpeed    = 0.0f;
    float  playerRadius = 0.0f;
    bool   justLanded   = false;
    bool   parked       = false;
    Vector listener     = {0, 0, 0};

    // In-range warn-loop hazards, before the nearest-N cut.
    struct Candidate {
        int         slot;
        float       dist;
        Vector      pos;
        const char* name;
    };
    Candidate candidates[kMgoArraySlotCount];
    int candidateCount = 0;

    // This tick's bars (K2), published to g_state after the sweep so the pair
    // lookup inside it reads a consistent (last tick's) list.
    SpatialAudioState::BarInfo newBars[kMgoArraySlotCount];
    int newBarCount = 0;

    int slotsSeen = 0, obstaclesFound = 0, ahead = 0, inRange = 0;
    int postsSkipped = 0, scenerySkipped = 0, barsSeen = 0, minesInRange = 0;
};

// One obstacle as the steps see it. K2 boxes are unrotated on straight +Y
// tracks (the axis probe has asserted +Y), so the world box is the origin
// plus the local box.
struct ObstacleView {
    void*       obstacle  = nullptr;
    Vector      pos       = {0, 0, 0};
    const char* name      = nullptr;
    bool        haveBox   = false;
    float       x0 = 0.0f, x1 = 0.0f, y0 = 0.0f, y1 = 0.0f, z0 = 0.0f;
    bool        scenery   = false;
    bool        post      = false;
    // Where the hazard really is: box centre laterally (clamped to the
    // lane), leading edge forward, base height.
    Vector      hazardPos = {0, 0, 0};
};

// minigame_swoop_audio_obstacles.cpp
void TickObstacleCues(void* miniGame);

// minigame_swoop_audio_bars.cpp
void TrackMinePasses(void* player, const Vector& bikePos, float playerRadius);
void HandleBar(int i, const ObstacleView& v, HazardInfo& hz, ObstacleTick& t);

// minigame_swoop_audio_accelpad.cpp
void TickAccelpadCues(void* miniGame);

}  // namespace acc::swoop_race

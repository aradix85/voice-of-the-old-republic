// Swoop race spatial audio, core: track-axis guard and the public entry
// points of minigame_swoop_audio.h.
//
// The race lifecycle (entry/exit, gear watch, wall collision,
// acceleration speedometer) lives in minigame_swoop_race.cpp; everything that
// walks the 255-slot CSWMiniGameObjectArray to surface obstacles and
// accelpads as 3D loops lives here.

#include "minigame_swoop_audio_internal.h"

#include "engine_game.h"
#include "log.h"
#include "menus_modsettings.h"

namespace acc::swoop_race {

namespace {

// ----- Object pool split: rocks vs accelerator pads -----
//
// Verified by extracting the per-track .are files (all three swoop
// modules — m03mg, m17mg, m26mg — agree):
//
//   <list label="Obstacles">  — exactly 22 entries (mgo01..mgo22).
//     Rocks / debris. CSWMGObstacle, vtable 0x0075287C. Names are
//     sequential (m03mg_MGO01..MGO22) and carry no semantic discriminator.
//
//   <list label="Enemies">    — 30 entries riding their own tracks
//     (mgt02..mgt31). All use model `mgf_accelpad01`, all carry
//     `<byte label="Trigger">1</byte>` (collision-only, no bullets).
//     These are the accelerator pads. CSWMiniEnemy (extends
//     CSWTrackFollower). On collision, the area-level
//     OnHitFollower=accelpad script (tar_m03mg_s.rim/accelpad.ncs)
//     fires SWMG_SetPlayerSpeed * 1.05 + acceleration * 1.10.
//
// CSWMGObstacle::GetName @0x66dc80 reads its visual name by calling
// CAurObject's vtable[0xc]. Retained for the first-fire diagnostic
// dump so the obstacle inventory is still logged on entry.
constexpr size_t kAurVtableSlotGetName          = 0xc;

// CAurObject vtable[+0xc] — returns the obstacle's name string (lives
// in the underlying Gob/Model — same accessor sighted UI uses via
// CSWMGObstacle::GetName).
typedef const char* (__thiscall* PFN_GetAurName)(void* this_);

// ============================================================================
// Track-axis guard.
// ============================================================================
//
// Everything in this file assumes the track's FORWARD direction is world +Y and
// its LANE axis is world X. That is a KOTOR 1 measurement, not an engine
// guarantee: its three swoop tunnels run +Y and the listener's Y climbs to
// ~4000 across a race, which is what the "obstacles behind us" filter, the
// next-gate pick and the fixed-depth pan construction were all built on. The
// three KOTOR 2 tracks (211TEL / 371NAR / 510OND) are different geometry and
// nothing offline says which way they run — their obstacles are named scene
// objects whose positions live in the room models, not in the area GFF.
//
// Firing this module on a track that runs some other way would not degrade
// gracefully: "ahead" would select obstacles BEHIND the player and the pan
// would point at the wrong lane — confidently wrong cues, the worst outcome for
// a screen-reader user. So measure instead of assume. Sample the bike's world
// position once the race is moving, and when it has travelled far enough for
// the direction to be unambiguous, require +Y to dominate. A track that fails
// leaves these cues silent and says so once in the log.
//
// Deliberately narrow: only the geometry-dependent cues gate on this. Race
// entry/exit, the gear watch, the shift-ready cue and the race clock read no
// world geometry at all and keep working on any track.
//
// KOTOR 1 is unaffected in practice — its tracks pass — but this runs there
// too, so the probe itself is a K1 regression item.
constexpr float kAxisProbeTravelU = 60.0f;  // travel needed before deciding
constexpr float kAxisProbeDominance = 2.0f; // +Y must beat |X| and |Z| by this

bool TrackAxisUsable(void* miniGame) {
    if (g_state.axis_state == kAxisRejected) return false;
    if (g_state.axis_state == kAxisAccepted) return true;

    void* player = ReadPtrOrNull(miniGame, kMiniGamePlayerOffset);
    Vector pos;
    if (!ReadFollowerPosition(player, pos)) return false;  // undecided → silent

    if (!g_state.have_axis_origin) {
        g_state.axis_origin      = pos;
        g_state.have_axis_origin = true;
        return false;
    }

    const float dx = pos.x - g_state.axis_origin.x;
    const float dy = pos.y - g_state.axis_origin.y;
    const float dz = pos.z - g_state.axis_origin.z;
    const float travel = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (travel < kAxisProbeTravelU) return false;  // still deciding

    const bool forwardIsPlusY =
        dy > 0.0f &&
        dy > kAxisProbeDominance * std::fabs(dx) &&
        dy > kAxisProbeDominance * std::fabs(dz);

    g_state.axis_state = forwardIsPlusY ? kAxisAccepted : kAxisRejected;
    acclog::Write("SwoopRace",
                  "track axis probe: travel=(%.1f,%.1f,%.1f) len=%.1f -> %s",
                  dx, dy, dz, travel,
                  forwardIsPlusY ? "+Y forward, spatial cues ON"
                                 : "NOT +Y forward, spatial cues OFF "
                                   "(obstacle / accelpad geometry is +Y-only)");
    return forwardIsPlusY;
}

}  // namespace

// Returns the CAurObject's name pointer (engine-owned, valid for the
// lifetime of the object), or nullptr on any null link / SEH fault.
const char* ReadAurObjectName(void* aurObject) {
    const auto getName = reinterpret_cast<PFN_GetAurName>(
        acc::engine::ReadVirtual(aurObject, kAurVtableSlotGetName));
    if (!getName) return nullptr;
    __try {
        return getName(aurObject);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

// ============================================================================
// Public entry points.
// ============================================================================

void TickSpatialAudio(void* miniGame) {
    if (!TrackAxisUsable(miniGame)) return;
    // Pads first: that sweep also finds the K2 mines, which the obstacle sweep
    // then sounds as blocks.
    TickAccelpadCues(miniGame);
    TickObstacleCues(miniGame);
}

ParkedBar GetParkedBarSituation() {
    return g_state.parked_bar;
}

bool AutopilotOn() {
    return acc::game::IsKotor1() &&
           acc::menus::modsettings::GetToggle(
               acc::menus::modsettings::Option::SwoopAutopilot);
}

void ResetSpatialAudio() {
    g_state.obstacle_diag_emitted = false;
    g_state.accelpad_diag_emitted = false;
    g_state.mine_count            = 0;
    g_state.was_airborne          = false;
    g_state.parked_bar            = ParkedBar::None;
    for (int i = 0; i < kMgoArraySlotCount; ++i) {
        g_state.hazards[i].state       = kHazardUntried;
        g_state.hazards[i].cls         = kHazardBlock;
        g_state.hazards[i].from_part   = 0;
        g_state.hazards[i].jump_cued   = 0;
        g_state.hazards[i].park_cued   = 0;
    }
    g_state.prev_bike_x           = 0.0f;
    g_state.have_prev_bike_x      = false;
    g_state.smoothed_vel          = 0.0f;
    g_state.prev_ahead_slot       = -1;
    g_state.guide_was_active      = false;
    g_state.last_wall_cue_ms      = 0;
    g_state.wall_loop.Stop();
    g_state.wall_loop_side        = 0;
    g_state.detour_slot           = -1;
    g_state.bar_count             = 0;
    g_state.bar_detour_slot       = -1;
    g_state.bar_air_cleared_slot  = -1;
    g_state.bar_gap_diag_emitted  = false;
    g_state.ground_z              = 40.0f;
    for (int i = 0; i < kMgoArraySlotCount; ++i) g_state.mine_pass_open[i] = false;
    g_state.last_magnet_log_ms    = 0;
    g_state.have_prev_ahead       = false;
    g_state.prev_ahead_posX       = 0.0f;
    g_state.prev_ahead_posY       = 0.0f;
    g_state.prev_ahead_padErr     = 0.0f;
    g_state.prev_ahead_fwdGap     = 0.0f;
    g_state.axis_state            = kAxisUndecided;
    g_state.have_axis_origin      = false;
    g_state.accelpad_line_loop.Stop();
    for (int i = 0; i < kMgoArraySlotCount; ++i) {
        g_state.obstacle_loops[i].Stop();
    }
}

}  // namespace acc::swoop_race

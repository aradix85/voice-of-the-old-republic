// The swoop race's spoken cues during a race: gear changes, when to shift, hit points, a stuck bike, and jumps.

#include "minigame_swoop_race_internal.h"

#include "engine_area.h"      // GetClientArea + map-pin chain (back-pointer),
                              //   ReadGlobalNumber (MIN_RACE_GEAR / MIN_TIME_*)
#include "engine_game.h"      // IsKotor2 — gate ladder, jump watch, K2 keys
#include "engine_player.h"    // GetCameraPosition (jump-start log anchor)
#include "log.h"
#include "audio_bus.h"        // PlayCue — non-positional "you can shift now" cue
#include "audio_cues.h"       // NavCue + GetNavCueResref (shift-ready resref)
#include "speech.h"
#include "strings.h"          // Get(SwoopRaceStarted/Controls/Ended) +
                              //     FmtSwoopRaceGear

namespace acc::swoop_race {

namespace {

// The obstacle / enemy position offsets, the vtable downcasts and the
// CSWMiniGameObjectArray layout live with the per-tick audio sweeps in
// minigame_swoop_audio*.cpp.
//
// There is no analog throttle in swoop (CSWMiniPlayer::Control, onfire.ncs):
// the bike accelerates to max_speed on its own, and each accelerate press is
// a discrete shift (onfire raises max_speed one notch, 35→60→100→150→210,
// with the native Shift1/2/3 + Engine0N sounds).
// ============================================================================
// Tunable behaviour parameters.
// ============================================================================
// Minimum jump in max_speed that counts as a "shift up" event.
//
// The engine itself has NO gear concept (Ghidra-confirmed 2026-05-24
// against CSWMiniPlayer::Control @0x66d640): the per-tick movement
// function only clamps speed against max_speed, and max_speed is
// settable only via the NWScript SWMG_SetPlayerMaxSpeed dispatcher
// (CSWMiniPlayer::SetMaxSpeed @0x66cf70 has exactly one caller,
// ExecuteCommandSWMG_SetPlayerFloatInfo @0x5cce70). So the entire
// gear ladder lives in the per-track NWScript — different swoop
// tracks may have different gear counts and different speed bands.
//
// Empirical floor from the 2026-05-24 race (tar_m03mg, gear 1 max=70,
// gear 2 max=120, delta 50; same-gear jitter < 1 m/s). 5 m/s is well
// above the jitter and well below the smallest plausible shift.
constexpr float kGearShiftMaxDeltaMs      = 5.0f;

// ----- "You can shift now" cue (Option A — exact onfire.ncs gate) -----
//
// onfire.ncs (the manual-accelerate StartingConditional, decompiled from
// build/swoop-rim/onfire.ncs 2026-06-22) only upshifts from gear N when the
// CURRENT speed already exceeds gear N+1's min-speed; below that the accelerate
// press is a SILENT no-op. So the earliest legal shift moment is the frame
// speed first crosses that gate. We fire a one-shot cue there so the player can
// shift as early as possible — the gate always sits BELOW the gear's own
// max_speed, so you never have to redline to upshift.
//
// The gate ladder lives in per-track NWScript, but all three shipped swoop
// tracks share it (Taris/Tatooine/Manaan: gear min-speeds 35/60/100/150/210;
// only the per-track *_SWOOP_ACCEL accel rate differs). Indexed by the CURRENT
// gear; the value is the speed you must EXCEED to upshift out of it. 0 = no
// gate (gear 0 launch is unconditional; gear 5 is top). Gears 1..4 are gated.
constexpr float kShiftGateSpeedK1[]       = { 0.0f, 60.0f, 100.0f, 150.0f,
                                              210.0f, 0.0f };
// KOTOR 2 uses the SAME ladder. A stack accounting of its onaccelerate.ncs
// (2026-09-14) read the gates one notch lower (35 / 60 / 100 / 150) and two
// rounds ran on that; the logs refuted it (rounds 4-5, patch-20260915-075412 /
// -083506): every shift the engine accepted landed at 70 / 105-110 / 155 /
// 214, and presses at 35-45 (gear 1), 76-86 (gear 2), 115-150 (gear 3),
// 158-200 (gear 4) ran the script with no gear change. The gate out of gear
// N is the NEXT gear's nominal minimum — 60 / 100 / 150 / 210 — i.e. the top
// 5-10 units of each gear's envelope (gear 1 [35f..70], gear 2 [60f..110],
// gear 3 [100f..155], gear 4 [150f..215]). The wrong table made the cue fire
// up to 40 units early, the player pressed into a no-op and then heard the
// next gear's cue right after the engine's own shift sound.
constexpr float kShiftGateSpeedK2[]       = { 0.0f, 60.0f, 100.0f, 150.0f,
                                              210.0f, 0.0f };
constexpr int   kFirstGatedGear           = 1;
constexpr int   kLastGatedGear            = 4;
constexpr ULONGLONG kShiftReadyHoldMs     = 800;   // see TickShiftReady

// Resref centralised in audio_cues.h so the audio glossary auditions the same
// sample.
constexpr const char* kShiftReadyCueResref =
    acc::audio::GetNavCueResref(acc::audio::NavCue::SwoopShiftReady);

// ============================================================================
// Stuck hint. The first K2 round ended pinned for 20 s: a bar hit had bounced
// the bike against a lane-edge post, and every relaunch (speed ~6) ran straight
// into it again — gear 1, gear 0, gear 1, gear 0 — while W-jumps from a
// standstill went nowhere. Nothing in the audio said what to do. When the
// race is running, the gear is 0 and the bike has not gained ground for
// kStuckHoldMs, speak the escape once; re-arm after real progress.
// ============================================================================
constexpr float     kStuckProgressU = 15.0f;   // forward gain that counts as progress
constexpr ULONGLONG kStuckHoldMs    = 6000;    // no progress for this long -> hint
// Parked right in front of a low bar (GetParkedBarSituation, KOTOR 2): the
// situation is unambiguous and the escape is a fixed key sequence, so the
// hint comes after this much instead — round 16 (patch-20260915-212249)
// spent up to 11 relaunches, 50 hp each, waiting for the six-second hint,
// and the hint then said "steer left, then W" without the jump.
constexpr ULONGLONG kStuckBarHoldMs = 1500;

constexpr float kStuckWallU = 13.0f;   // |tunnel x| at/beyond this = parked at a wall

// ============================================================================
// Jump watch (KOTOR 2). Log-only for now: start + landing edges with the
// numbers a "jump now" cue needs (speed, gear, air time, peak height, world
// position). K1 never arms — its tunnel clamps offset.z to 0.
// ============================================================================
constexpr float kJumpArmZ    = 0.25f;  // offset.z above this = airborne
constexpr float kJumpLandZ   = 0.05f;  // ... and back below this = landed

inline float ShiftGateSpeed(int gear) {
    return acc::game::IsKotor2() ? kShiftGateSpeedK2[gear]
                                 : kShiftGateSpeedK1[gear];
}

// ============================================================================
// Gear-change detection.
// ============================================================================

// Speak the gear the bike is now in and re-arm the shift-ready cue for it.
// Shared by both gear sources. `why` is log-only.
void AnnounceGear(int gear, const char* why) {
    g_state.gear                  = gear;
    g_state.shift_ready_announced = false;
    g_state.last_gear_change_ms   = GetTickCount64();
    char buf[64];
    const char* fmt = acc::strings::Get(acc::strings::Id::FmtSwoopRaceGear);
    if (fmt && *fmt) {
        std::snprintf(buf, sizeof(buf), fmt, gear);
        acc::speech::Speak(buf, /*interrupt=*/true);
    }
    acclog::Write("SwoopRace", "gear -> %d (%s)", gear, why);
}

}  // namespace

void TickGearWatch(void* miniGame) {
    // ---- Primary: the track scripts' own MIN_RACE_GEAR (see kGearGlobalName).
    // Announced on EVERY change, downward included — on KOTOR 2 an obstacle
    // hit drops the bike to gear 0 and it has to be re-launched, which is
    // exactly the moment the player needs to hear "gear 0".
    const int scriptGear = acc::engine::ReadGlobalNumber(kGearGlobalName);
    if (scriptGear >= kGearGlobalMin && scriptGear <= kGearGlobalMax) {
        if (!g_state.gear_from_global) {
            g_state.gear_from_global = true;
            acclog::Write("SwoopRace",
                          "gear source: %s global (reads %d; plateau "
                          "heuristic off for this race)",
                          kGearGlobalName, scriptGear);
        }
        if (scriptGear != g_state.gear) {
            char why[48];
            std::snprintf(why, sizeof(why), "%s %d -> %d",
                          kGearGlobalName, g_state.gear, scriptGear);
            AnnounceGear(scriptGear, why);
        }
        return;
    }
    if (g_state.gear_from_global) return;  // countdown/finish value — keep last

    // ---- Fallback: max_speed-plateau heuristic (kGearShiftMaxDeltaMs), for a
    // track whose scripts do not maintain the global. Upward jumps only.
    void* player = SafeReadPtr(miniGame, kMiniGamePlayerOffset);
    if (!player) return;

    float maxSpeed = SafeReadFloat(player, kMiniPlayerMaxSpeedOffset);
    if (maxSpeed <= 0.0f) return;  // not yet initialised

    // Shift-up event: max_speed jumped by at least the noise floor.
    if (maxSpeed > g_state.last_max_speed + kGearShiftMaxDeltaMs) {
        char why[64];
        std::snprintf(why, sizeof(why), "plateau max=%.2f was %.2f",
                      maxSpeed, g_state.last_max_speed);
        g_state.last_max_speed = maxSpeed;
        AnnounceGear(g_state.gear + 1, why);
    }
}

// ============================================================================
// Hit points (log only). K2's enemyhitfollower.ncs subtracts 50 per obstacle
// collision, mine.ncs 100 per mine, both from the Citadel bike's 600; the
// heartbeat ends the race when the percentage it keeps in the racedialog
// local reaches 0 — round 3 (patch-20260914-204103) took exactly twelve hits
// and the run ended at 72 s, which the mod announced as a finish time. Each
// drop is logged; nothing is spoken here (round 4's "hit, N left" per drop
// was cut as noise — the HUD's damage gauge is the sighted equivalent). The
// wreck itself is decided at the race end (DecideWreckAtFinish): the engine
// mirrors a script's hit-point write late — round 4 read 95 for three
// seconds after the mine had zeroed the bike — so a live zero cannot be the
// trigger, and the follower reads 0 once more when it is torn down after
// the race. K1's obstacle script never touches hp, so nothing fires there.
// ============================================================================

void TickHitPoints(void* miniGame) {
    void* player = SafeReadPtr(miniGame, kMiniGamePlayerOffset);
    if (!player) return;
    const int hp = static_cast<int>(SafeReadU32(player, kFollowerHpOffset));
    if (hp < 0 || hp > 1000000) return;              // faulted / garbage
    // After the finish the engine zeroes the follower (max hp 0 too): that
    // final "hp 400 -> 0 (drop 400 of 0)" is not a hit, so keep it out of the
    // per-race hit count a log reader would tally.
    if (SafeReadU32(player, kFollowerMaxHpOffset) == 0) return;
    if (g_state.last_hp < 0) { g_state.last_hp = hp; return; }
    if (hp >= g_state.last_hp) { g_state.last_hp = hp; return; }
    const int drop = g_state.last_hp - hp;
    g_state.last_hp = hp;
    acclog::Write("SwoopRace", "hit: hp %d -> %d (drop %d of %u)",
                  hp + drop, hp, drop, SafeReadU32(player, kFollowerMaxHpOffset));
}

void TickStuckHint(void* miniGame) {
    if (!g_state.have_start_ms) return;        // not launched yet / countdown
    Vector cam = {0.0f, 0.0f, 0.0f};
    if (!acc::engine::GetCameraPosition(cam)) return;
    const ULONGLONG now = GetTickCount64();
    if (!g_state.have_progress_mark ||
        cam.y - g_state.progress_mark_y >= kStuckProgressU) {
        g_state.have_progress_mark   = true;
        g_state.progress_mark_y      = cam.y;
        g_state.progress_mark_ms     = now;
        g_state.stuck_hint_spoken    = false;    // progress re-arms the hint
        g_state.stuck_bar_spoken_for = ParkedBar::None;
        return;
    }
    if (g_state.race_time_announced) return;   // finished, not stuck
    void* player = SafeReadPtr(miniGame, kMiniGamePlayerOffset);
    const float speed = player ? SafeReadFloat(player, kFollowerSpeedOffset) : 0.0f;
    const int   gear  = acc::engine::ReadGlobalNumber(kGearGlobalName);
    if (gear != 0 || speed > 2.0f) return;     // moving or in gear: not pinned
    const ParkedBar bar  = GetParkedBarSituation();
    const ULONGLONG hold = (bar != ParkedBar::None) ? kStuckBarHoldMs : kStuckHoldMs;
    if (now - g_state.progress_mark_ms < hold) return;
    // Once per situation: a hint already spoken for THIS situation stays
    // spoken until progress; a changed situation (off the wall now, or the
    // bar classified after a generic hint) gets its own.
    if (g_state.stuck_hint_spoken && bar == g_state.stuck_bar_spoken_for) return;
    g_state.stuck_hint_spoken    = true;
    g_state.stuck_bar_spoken_for = bar;
    // At a wall the direction is known (K2 posts from |x| ≈ 16.3; 13 leaves
    // the pad lanes at 11 out of it): say which way, then the shift key.
    Vector off = {0.0f, 0.0f, 0.0f};
    const bool offOk = player && SafeReadVector(player, kMiniPlayerOffsetVectorOffset, off);
    acc::strings::Id id = acc::strings::Id::SwoopRaceStuckHint;
    const char* why = "generic";
    switch (bar) {
        case ParkedBar::WallRight: id = acc::strings::Id::SwoopRaceStuckBarWallLeft;  why = "bar, wall right"; break;
        case ParkedBar::WallLeft:  id = acc::strings::Id::SwoopRaceStuckBarWallRight; why = "bar, wall left";  break;
        case ParkedBar::Strip:     id = acc::strings::Id::SwoopRaceStuckBarStrip;     why = "bar, strip";      break;
        case ParkedBar::MustJump:  id = acc::strings::Id::SwoopRaceStuckBarJump;      why = "bar, must jump";  break;
        case ParkedBar::None:
            if (offOk && off.x >= kStuckWallU)       { id = acc::strings::Id::SwoopRaceStuckHintLeft;  why = "wall right"; }
            else if (offOk && off.x <= -kStuckWallU) { id = acc::strings::Id::SwoopRaceStuckHintRight; why = "wall left"; }
            break;
    }
    const char* msg = acc::strings::Get(id);
    if (msg && *msg) acc::speech::Speak(msg, /*interrupt=*/true);
    acclog::Write("SwoopRace", "stuck hint: no progress for %llums at y=%.1f "
                  "(gear=%d speed=%.1f tunnelX=%.1f) [%s]",
                  now - g_state.progress_mark_ms, cam.y, gear, speed,
                  offOk ? off.x : 0.0f, why);
}

void TickJumpWatch(void* miniGame) {
    if (!acc::game::IsKotor2()) return;
    void* player = SafeReadPtr(miniGame, kMiniGamePlayerOffset);
    if (!player) return;
    Vector off;
    if (!SafeReadVector(player, kMiniPlayerOffsetVectorOffset, off)) return;

    if (!g_state.airborne) {
        if (off.z <= kJumpArmZ) return;
        g_state.airborne        = true;
        g_state.jump_started_ms = GetTickCount64();
        g_state.jump_peak_z     = off.z;
        const float speed = SafeReadFloat(player, kFollowerSpeedOffset);
        const float vz    = SafeReadFloat(miniGame, kMiniGameJumpVelocityOffset);
        Vector pos = {0.0f, 0.0f, 0.0f};
        const bool posOk = acc::engine::GetCameraPosition(pos);
        acclog::Write("SwoopRace",
                      "jump start: z=%.2f vz=%.1f speed=%.1f gear=%d "
                      "camera=(%.1f,%.1f,%.1f)%s",
                      off.z, vz, speed, g_state.gear,
                      pos.x, pos.y, pos.z, posOk ? "" : " (no camera)");
        return;
    }
    if (off.z > g_state.jump_peak_z) g_state.jump_peak_z = off.z;
    if (off.z >= kJumpLandZ) return;
    g_state.airborne = false;
    const ULONGLONG air = GetTickCount64() - g_state.jump_started_ms;
    acclog::Write("SwoopRace",
                  "jump end: air=%llums peak_z=%.2f gear=%d",
                  air, g_state.jump_peak_z, g_state.gear);
}

// ============================================================================
// "You can shift now" cue (Option A — exact onfire.ncs gate, see
// kShiftGateSpeed).
// ============================================================================
//
// Fires a one-shot cue the frame current speed first crosses the gate for the
// current gear, i.e. the earliest frame a manual shift would actually take.
// Latched per gear (TickGearWatch re-arms it on each real shift). Self-guards
// against a non-stock gate ladder: never announces a gate that sits at or above
// the gear's own max_speed (an unreachable gate the bike can't climb to), so a
// mod track with a different ladder degrades to silence, never a false cue.
void TickShiftReady(void* miniGame) {
    if (g_state.shift_ready_announced) return;
    // Hold after a gear change. On KOTOR 2 a late shift (at the old gear's
    // top speed) is already above the NEXT gate, so the cue fired in the same
    // tick as the "gear N" speech and read as a shift confirmation
    // (round 3, 2026-09-14). A short gap keeps it a separate event; on K1 the
    // gates sit a notch higher so the hold never applies.
    if (GetTickCount64() - g_state.last_gear_change_ms < kShiftReadyHoldMs) return;

    const int gear = g_state.gear;
    if (gear < kFirstGatedGear || gear > kLastGatedGear) return;  // launch / top gear: no gate
    const float gate = ShiftGateSpeed(gear);
    if (gate <= 0.0f) return;

    void* player = SafeReadPtr(miniGame, kMiniGamePlayerOffset);
    if (!player) return;

    const float maxSpeed = SafeReadFloat(player, kMiniPlayerMaxSpeedOffset);
    if (maxSpeed <= 0.0f || gate >= maxSpeed) return;  // self-guard: gate unreachable

    const float speed = SafeReadFloat(player, kFollowerSpeedOffset);
    if (speed <= gate) return;

    g_state.shift_ready_announced = true;
    acc::audio::PlayCue(kShiftReadyCueResref);
    acclog::Write("SwoopRace",
                  "shift-ready cue: gear=%d speed=%.2f > gate=%.2f (max=%.2f)",
                  gear, speed, gate, maxSpeed);
}

}  // namespace acc::swoop_race

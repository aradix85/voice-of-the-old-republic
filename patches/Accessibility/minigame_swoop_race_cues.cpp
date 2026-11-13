// The swoop race's cues during a race: gear changes, when to shift, hit
// points, a stuck bike, and jumps.

#include "minigame_swoop_race_internal.h"

#include "audio_bus.h"
#include "audio_cues.h"
#include "engine_area.h"
#include "engine_game.h"
#include "engine_player.h"
#include "log.h"
#include "speech.h"
#include "strings.h"

namespace acc::swoop_race {

namespace {

// There is no analog throttle in swoop (CSWMiniPlayer::Control, onfire.ncs):
// the bike accelerates to max_speed on its own, and each accelerate press is
// a discrete shift (onfire raises max_speed one notch, 35→60→100→150→210,
// with the native Shift1/2/3 + Engine0N sounds).
//
// Minimum jump in max_speed that counts as a shift, for the plateau
// heuristic. The engine itself has NO gear concept (CSWMiniPlayer::Control
// @0x66d640 only clamps speed against max_speed, and max_speed is set only
// through the NWScript SWMG_SetPlayerMaxSpeed dispatcher: SetMaxSpeed
// @0x66cf70 has exactly one caller, ExecuteCommandSWMG_SetPlayerFloatInfo
// @0x5cce70). So the gear ladder lives in the per-track NWScript. Measured
// on tar_m03mg: gear 1 max 70, gear 2 max 120, same-gear jitter < 1; 5 is
// well above the jitter and well below the smallest plausible shift.
constexpr float kGearShiftMinJump         = 5.0f;

// ----- "You can shift now" cue -----
//
// The accelerate script (onfire.ncs on KOTOR 1, onaccelerate.ncs on KOTOR 2)
// only upshifts from gear N when the CURRENT speed already exceeds gear N+1's
// nominal minimum; below that the press runs the script and changes nothing.
// The cue fires the frame speed first crosses that gate, so the player can
// shift as early as possible; the gate sits in the top of the gear's own
// envelope (KOTOR 2: gear 1 [35..70], gear 2 [60..110], gear 3 [100..155],
// gear 4 [150..215]), so no redlining is needed.
//
// The ladder lives in per-track NWScript, but every shipped track on both
// games uses the same one (min speeds 35/60/100/150/210; only the per-track
// *_SWOOP_ACCEL rate differs). A static read of K2's onaccelerate.ncs
// suggests gates one notch lower; the engine's accepted shifts (70 / 105-110
// / 155 / 214) and its no-op presses below 60 / 100 / 150 / 210 refute that.
// Indexed by the current gear: the speed to exceed to shift out of it.
constexpr float kShiftGateSpeed[]         = { 0.0f, 60.0f, 100.0f, 150.0f,
                                              210.0f, 0.0f };
constexpr int   kFirstGatedGear           = 1;
constexpr int   kLastGatedGear            = 4;
constexpr ULONGLONG kShiftReadyHoldMs     = 800;   // see TickShiftReady
constexpr ULONGLONG kAutoShiftRetryMs     = 400;   // see TickAutoShift

// Resref centralised in audio_cues.h so the audio glossary auditions the same
// sample.
constexpr const char* kShiftReadyCueResref =
    acc::audio::GetNavCueResref(acc::audio::NavCue::SwoopShiftReady);

// ----- Stuck hint -----
//
// On KOTOR 2 a bar hit can bounce the bike against a lane-edge post, and
// every relaunch (speed ~6) runs straight into it again while jumps from a
// standstill go nowhere. When the race is running, the gear is 0 and the
// bike has not gained ground for kStuckHoldMs, speak the escape once;
// re-arm after real progress.
constexpr float     kStuckProgressU = 15.0f;   // forward gain that counts as progress
constexpr ULONGLONG kStuckHoldMs    = 6000;    // no progress for this long -> hint
// Parked right in front of a low bar (GetParkedBarSituation, KOTOR 2): the
// situation is unambiguous, the escape is a fixed key sequence and every
// relaunch into the bar costs 50 hp, so the hint comes after this much.
constexpr ULONGLONG kStuckBarHoldMs = 1500;

constexpr float kStuckWallU = 13.0f;   // |tunnel x| at/beyond this = parked at a wall

// ----- Jump watch (KOTOR 2) -----
//
// Log only: start and landing edges with the numbers a "jump now" cue would
// need (speed, gear, air time, peak height, world position). K1 never arms —
// its tunnel clamps offset.z to 0.
constexpr float kJumpArmZ    = 0.25f;  // offset.z above this = airborne
constexpr float kJumpLandZ   = 0.05f;  // ... and back below this = landed

// Speak the gear the bike is now in and re-arm the shift-ready cue for it.
// Shared by both gear sources. `why` is log-only.
void AnnounceGear(int gear, const char* why) {
    g_state.gear                  = gear;
    g_state.shift_ready_announced = false;
    g_state.last_gear_change_ms   = GetTickCount64();
    char buf[64];
    std::snprintf(buf, sizeof(buf),
                  acc::strings::Get(acc::strings::Id::FmtSwoopRaceGear), gear);
    acc::speech::Speak(buf, /*interrupt=*/true);
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

    // ---- Fallback: the max_speed-plateau heuristic, for a track whose
    // scripts do not maintain the global. Upward jumps only.
    void* player = ReadPtrOrNull(miniGame, kMiniGamePlayerOffset);
    if (!player) return;

    float maxSpeed = ReadFloatOr0(player, kMiniPlayerMaxSpeedOffset);
    if (maxSpeed <= 0.0f) return;  // not yet initialised

    if (maxSpeed > g_state.last_max_speed + kGearShiftMinJump) {
        char why[64];
        std::snprintf(why, sizeof(why), "plateau max=%.2f was %.2f",
                      maxSpeed, g_state.last_max_speed);
        g_state.last_max_speed = maxSpeed;
        AnnounceGear(g_state.gear + 1, why);
    }
}

// Hit points, log only. K2's enemyhitfollower.ncs subtracts 50 per obstacle
// collision, mine.ncs 100 per mine, both from the Citadel bike's 600; the
// heartbeat ends the race when the percentage it keeps in the racedialog
// local reaches 0, after twelve obstacle hits. Nothing is spoken per drop
// (too noisy; the HUD's damage gauge is the sighted equivalent). The wreck
// itself is decided at the race end (DecideWreckAtFinish): the engine
// mirrors a script's hit-point write late — a bike a mine had zeroed still
// read 95 for three seconds — so a live zero cannot be the trigger, and the
// follower reads 0 once more when it is torn down after the race. K1's
// obstacle script never touches hp, so nothing fires there.
void TickHitPoints(void* miniGame) {
    void* player = ReadPtrOrNull(miniGame, kMiniGamePlayerOffset);
    if (!player) return;
    const int hp = static_cast<int>(ReadU32Or0(player, kFollowerHpOffset));
    if (hp < 0 || hp > 1000000) return;              // faulted / garbage
    // After the finish the engine zeroes the follower (max hp 0 too): that
    // final drop to 0 is not a hit, so keep it out of the per-race hit count
    // a log reader would tally.
    if (ReadU32Or0(player, kFollowerMaxHpOffset) == 0) return;
    if (g_state.last_hp < 0) { g_state.last_hp = hp; return; }
    if (hp >= g_state.last_hp) { g_state.last_hp = hp; return; }
    const int drop = g_state.last_hp - hp;
    g_state.last_hp = hp;
    acclog::Write("SwoopRace", "hit: hp %d -> %d (drop %d of %u)",
                  hp + drop, hp, drop, ReadU32Or0(player, kFollowerMaxHpOffset));
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
    void* player = ReadPtrOrNull(miniGame, kMiniGamePlayerOffset);
    const float speed = player ? ReadFloatOr0(player, kFollowerSpeedOffset) : 0.0f;
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
    const bool offOk = player && TryReadVector(player, kMiniPlayerOffsetVectorOffset, &off);
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
    acc::speech::Speak(acc::strings::Get(id), /*interrupt=*/true);
    acclog::Write("SwoopRace", "stuck hint: no progress for %llums at y=%.1f "
                  "(gear=%d speed=%.1f tunnelX=%.1f) [%s]",
                  now - g_state.progress_mark_ms, cam.y, gear, speed,
                  offOk ? off.x : 0.0f, why);
}

void TickJumpWatch(void* miniGame) {
    if (!acc::game::IsKotor2()) return;
    void* player = ReadPtrOrNull(miniGame, kMiniGamePlayerOffset);
    if (!player) return;
    Vector off;
    if (!TryReadVector(player, kMiniPlayerOffsetVectorOffset, &off)) return;

    if (!g_state.airborne) {
        if (off.z <= kJumpArmZ) return;
        g_state.airborne        = true;
        g_state.jump_started_ms = GetTickCount64();
        g_state.jump_peak_z     = off.z;
        const float speed = ReadFloatOr0(player, kFollowerSpeedOffset);
        const float vz    = ReadFloatOr0(miniGame, kMiniGameJumpVelocityOffset);
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

// The "you can shift now" cue (see kShiftGateSpeed): one cue the frame the
// current speed first crosses the gate for the current gear, i.e. the
// earliest frame a manual shift would take. Latched per gear (AnnounceGear
// re-arms it). Never announces a gate at or above the gear's own max_speed,
// so a modded track with a different ladder degrades to silence, never a
// false cue.
void TickShiftReady(void* miniGame) {
    if (g_state.shift_ready_announced) return;
    // Hold after a gear change. On KOTOR 2 a late shift (at the old gear's
    // top speed) is already above the NEXT gate, so the cue would fire in the
    // same tick as the "gear N" speech and read as a shift confirmation. A
    // short gap keeps it a separate event.
    if (GetTickCount64() - g_state.last_gear_change_ms < kShiftReadyHoldMs) return;

    const int gear = g_state.gear;
    if (gear < kFirstGatedGear || gear > kLastGatedGear) return;  // launch / top gear: no gate
    const float gate = kShiftGateSpeed[gear];

    void* player = ReadPtrOrNull(miniGame, kMiniGamePlayerOffset);
    if (!player) return;

    const float maxSpeed = ReadFloatOr0(player, kMiniPlayerMaxSpeedOffset);
    if (maxSpeed <= 0.0f || gate >= maxSpeed) return;  // gate unreachable

    const float speed = ReadFloatOr0(player, kFollowerSpeedOffset);
    if (speed <= gate) return;

    g_state.shift_ready_announced = true;
    acc::audio::PlayCue(kShiftReadyCueResref);
    acclog::Write("SwoopRace",
                  "shift-ready cue: gear=%d speed=%.2f > gate=%.2f (max=%.2f)",
                  gear, speed, gate, maxSpeed);
}

// The automatic pilot's shifts: the launch out of gear 0, and each gear as
// soon as TickShiftReady has seen its gate. A press can be lost, so it is
// repeated every kAutoShiftRetryMs until the gear has changed. The launch
// waits for nothing but the gear global reading 0, which is the "Go!": the
// race timer only starts once the bike moves.
void TickAutoShift() {
    if (!AutopilotOn() || g_state.race_time_announced) return;
    const bool launch = g_state.gear_from_global && g_state.gear == 0;
    if (!launch && !g_state.shift_ready_announced) return;
    const ULONGLONG now = GetTickCount64();
    if (now - g_state.last_auto_shift_ms < kAutoShiftRetryMs) return;
    g_state.last_auto_shift_ms = now;
    acclog::Write("SwoopRace", "pilot: shift out of gear %d", g_state.gear);
    TryShift();
}

}  // namespace acc::swoop_race

// The aim assist: the fighter's motion estimate, the predictive lead, the
// offset-sign calibration, steering the gun, and full-autoaim's retargeting.

#include "minigame_turret_internal.h"

#include "log.h"
#include "minigame_magnet.h"

namespace acc::turret_game {

namespace {

// The fighter's motion, for the lead (minigame_turret_lead.h). Velocity is an
// EMA of the per-tick position change. The fighters hold about constant speed
// and only turn, so their acceleration is taken as the perpendicular part of
// d(velocity)/dt (the along-track part is noise), clamped, and smoothed
// harder than the velocity since a second derivative is noisier.
constexpr float kVelEmaAlpha         = 0.25f;
constexpr float kAccelEmaAlpha       = 0.12f;
constexpr int   kAccelSettleSamples  = 5;      // ticks before the turn estimate is trusted
constexpr float kMaxCentripetalAccel = 300.0f; // u/s^2 sanity clamp on the estimate

// ----- Aim-assist (magnetism — the DEFAULT, always-on mode) -----
// Steering writes the aim field directly: offset.z (azimuth) and offset.x
// (elevation) at CSWMiniPlayer +0x1c4 (engine-confirmed — Control re-integrates
// it each tick, so a per-tick write sticks; see ReadAimLine / WriteOffset).
//
// CONSOLE-STYLE STICKY AIM: pure by-ear aiming is about unplayable (8%
// on-target in a measured round) because fast crossing fighters can't be
// tracked manually, and a narrow, yield-on-WASD magnetism almost never
// engages. The pull:
//   * engages from WIDE angles (kMagnetEngageDeg), so it starts guiding long
//     before you're on target;
//   * applies EVERY tick and BLENDS with your own WASD (no yield-on-keypress) —
//     your swing moves the aim, the pull refines on top, so it nudges while you
//     hold a key;
//   * RAMPS with proximity — gentle far out (a guide you can swing straight
//     through to retarget a different fighter), strong near the cone (the gun
//     sticks: the console bullet-magnetism feel);
//   * is per-tick CAPPED so a far target is pulled, not yanked.
// Beyond the engage angle (and behind you) there's no pull — you swing / Q-E onto
// a target yourself, keeping target choice in your hands. The "Autoaiming" toggle
// (TurretAutoAim) is the separate, unconditional full lock-on. Tune by feel.
constexpr float kAssistGainAuto   = 0.85f;  // full-autoaim: snap hard each tick
constexpr float kMagnetEngageDeg  = 60.0f;  // magnetism starts pulling within this aim error
constexpr float kMagnetGainFar    = 0.04f;  // pull gain at the engage edge (gentle guide)
constexpr float kMagnetGainNear   = 0.50f;  // pull gain dead-on (sticky)
constexpr float kMagnetMaxStepDeg = 6.0f;   // per-tick cap (deg) on the magnet pull

// ----- Makeability gate (FULL-AUTOAIM ONLY) -----
// "Makeable" = can a bolt actually connect this tick? The lead already cancels
// the crossing drift (LeadCheck proved ~80% of shots land inside the hitbox at
// every range, out to 400 m+), so the limiter is NOT crossing·range — that's the
// UN-led miss, which over-rejects slow-but-far fighters the lead hits cleanly.
// The real limiter is the EXTRAPOLATION HORIZON: the further ahead we predict
// (longer bolt flight), the more the estimate drifts. So gate on flight time.
// This is purely a TARGETING decision (which fighter is worth shooting / when
// the cue may say "fire"), confined to full-autoaim — assisted mode leaves that
// call to the player. When the locked fighter stays unmakeable past the debounce
// (and we're outside the manual-pick grace AND the post-switch cooldown), autoaim
// hands the lock to the nearest fighter — nearest = shortest flight = most
// reliable + fastest kill — instead of chasing a far target. The cooldown is the
// anti-thrash lever: each switch resets the lead to cold, so we MUST commit long
// enough for it to re-warm, or the gun thrashes between targets hitting nothing.
// The flight-time limit is MakeHorizonSec (range model above; 1.6 s on K1).
constexpr ULONGLONG kRetargetDebounceMs = 400;    // unmakeable this long before switching
constexpr ULONGLONG kManualGraceMs      = 2000;   // respect a manual Q/E pick this long
constexpr ULONGLONG kRetargetCooldownMs = 1500;   // commit after a switch (let the lead re-warm)

// ----- Switch-on-hit damage spreading (FULL-AUTOAIM ONLY) -----
// Damage is invulnerability-gated: a fighter that just took a hit ignores further
// bolts for a short window (the losing runs landed only ~half their connecting
// bolts — the rest hit invuln frames). So concentrating fire on one target wastes
// half of it while the other fighters shoot you. The instant autoaim lands a hit,
// hand the lock to the nearest OTHER fighter that isn't itself in its invuln
// window — spreading damage across the wave the way manual forward-spam does,
// which is why spam wins and lock-on-one loses. This bypasses the commit cooldown
// (the whole point is to move on a hit), but still honours a manual Q/E grace.
constexpr ULONGLONG kInvulnWindowMs = 1200;  // a fighter hit within this is still invulnerable

}  // namespace

// ---- Smoothed relative velocity (Δ relative-pos / Δt, EMA). ------------
void UpdateMotionEstimate(const Vector& P, ULONGLONG now) {
    if (g_state.have_last_pos && now > g_state.last_selected_pos_ms) {
        const float dt = (now - g_state.last_selected_pos_ms) / 1000.0f;
        const Vector vinst = {(P.x - g_state.last_selected_pos.x) / dt,
                              (P.y - g_state.last_selected_pos.y) / dt,
                              (P.z - g_state.last_selected_pos.z) / dt};
        if (g_state.have_vel) {
            const Vector velPrev = g_state.vel_ema;
            const float a = kVelEmaAlpha;
            g_state.vel_ema.x = a * vinst.x + (1.0f - a) * g_state.vel_ema.x;
            g_state.vel_ema.y = a * vinst.y + (1.0f - a) * g_state.vel_ema.y;
            g_state.vel_ema.z = a * vinst.z + (1.0f - a) * g_state.vel_ema.z;

            // Centripetal acceleration = perpendicular part of d(vel_ema)/dt.
            // Drop the along-track component (speed is ~constant, so it's noise),
            // clamp the magnitude, then EMA harder than velocity.
            if (dt > 1e-4f) {
                Vector ai = {(g_state.vel_ema.x - velPrev.x) / dt,
                             (g_state.vel_ema.y - velPrev.y) / dt,
                             (g_state.vel_ema.z - velPrev.z) / dt};
                const float vm = std::sqrt(g_state.vel_ema.x * g_state.vel_ema.x +
                                           g_state.vel_ema.y * g_state.vel_ema.y +
                                           g_state.vel_ema.z * g_state.vel_ema.z);
                if (vm > 1e-3f) {
                    const Vector vh = {g_state.vel_ema.x / vm,
                                       g_state.vel_ema.y / vm,
                                       g_state.vel_ema.z / vm};
                    const float adv = ai.x * vh.x + ai.y * vh.y + ai.z * vh.z;
                    ai.x -= adv * vh.x; ai.y -= adv * vh.y; ai.z -= adv * vh.z;
                }
                const float am = std::sqrt(ai.x * ai.x + ai.y * ai.y + ai.z * ai.z);
                if (am > kMaxCentripetalAccel) {
                    const float s = kMaxCentripetalAccel / am;
                    ai.x *= s; ai.y *= s; ai.z *= s;
                }
                const float b = kAccelEmaAlpha;
                g_state.accel_ema.x = b * ai.x + (1.0f - b) * g_state.accel_ema.x;
                g_state.accel_ema.y = b * ai.y + (1.0f - b) * g_state.accel_ema.y;
                g_state.accel_ema.z = b * ai.z + (1.0f - b) * g_state.accel_ema.z;
                if (++g_state.accel_samples >= kAccelSettleSamples)
                    g_state.have_accel = true;
            }
        } else {
            g_state.vel_ema = vinst;
            g_state.have_vel = true;
        }
    }
    g_state.last_selected_pos    = P;
    g_state.last_selected_pos_ms = now;
    g_state.have_last_pos        = true;
}

// Passive sign calibration: learn the sign of d(worldAim)/d(offset) per axis
// from frame-to-frame motion (player swing or our own writes). Voted for
// stability; target-independent (uses aim motion, not the target).
void CalibrateOffsetSigns(float offAz, float offEl, float aimAz, float aimEl) {
    if (g_state.calib_have) {
        const float dOffAz = Wrap180(offAz - g_state.calib_last_off_az);
        const float dAimAz = Wrap180(aimAz - g_state.calib_last_aim_az);
        if (std::fabs(dOffAz) > 1.0f && std::fabs(dAimAz) > 0.3f) {
            g_state.calib_vote_az += (dOffAz * dAimAz > 0.0f) ? 1 : -1;
            if (g_state.calib_vote_az >  3) g_state.calib_vote_az =  3;
            if (g_state.calib_vote_az < -3) g_state.calib_vote_az = -3;
            g_state.calib_sign_az = (g_state.calib_vote_az >= 0) ? 1 : -1;
        }
        const float dOffEl = offEl - g_state.calib_last_off_el;
        const float dAimEl = aimEl - g_state.calib_last_aim_el;
        if (std::fabs(dOffEl) > 1.0f && std::fabs(dAimEl) > 0.3f) {
            g_state.calib_vote_el += (dOffEl * dAimEl > 0.0f) ? 1 : -1;
            if (g_state.calib_vote_el >  3) g_state.calib_vote_el =  3;
            if (g_state.calib_vote_el < -3) g_state.calib_vote_el = -3;
            g_state.calib_sign_el = (g_state.calib_vote_el >= 0) ? 1 : -1;
        }
    }
    g_state.calib_last_off_az = offAz; g_state.calib_last_aim_az = aimAz;
    g_state.calib_last_off_el = offEl; g_state.calib_last_aim_el = aimEl;
    g_state.calib_have = true;
}

// Steer the gun by writing offset toward the locked intercept.
//  * Full-autoaim: snap hard, every tick.
//  * Magnetism (default): console-style sticky aim — proximity-ramped pull,
//    applied every tick and blended with the player's own WASD (no yield),
//    engaging out to kMagnetEngageDeg and capped per tick. See the tunables.
// Returns whether it steered; `gain` gets the gain used.
bool SteerGun(bool steer, float offAz, float offEl, float angle, float errAz,
              float errEl, bool fullAuto, float& gain) {
    const Arena& arena = g_state.arena;
    gain = 0.0f;
    if (steer && !fullAuto && angle > kMagnetEngageDeg)
        steer = false;  // target too far off-aim (or behind) — you swing onto it
    if (!steer) return false;

    const int sAz = g_state.calib_sign_az ? g_state.calib_sign_az : 1;
    const int sEl = g_state.calib_sign_el ? g_state.calib_sign_el : 1;
    // The error in offset units: the full correction would set the
    // offset to (current - error). errAz is the SHORTER way round in the
    // world, which on a bounded azimuth (107PER: -135..+70) can run
    // through the arc the gun cannot enter — the pull would then press
    // against the engine's clamp forever. When the shorter way leaves the
    // arc and the longer way stays inside it, take the longer way.
    float errAzOff = sAz * errAz;
    const float errElOff = sEl * errEl;
    const bool azBounded = arena.limits_read && !arena.az_free;
    const bool elBounded = arena.limits_read && !arena.el_free;
    if (azBounded) {
        const float want = offAz - errAzOff;
        if (want < arena.az_lo || want > arena.az_hi) {
            const float alt = want + (want < arena.az_lo ? 360.0f : -360.0f);
            if (alt >= arena.az_lo && alt <= arena.az_hi) errAzOff = offAz - alt;
        }
    }
    float newAz, newEl;
    if (fullAuto) {
        // Full lock-on (turret-only autoplay): snap hard, uncapped.
        gain  = kAssistGainAuto;
        newAz = offAz - errAzOff * gain;
        newEl = offEl - errElOff * gain;
    } else {
        // Magnetism (minigame_magnet.h): proximity-ramped
        // (t=0 at the engage edge, 1 dead-on), per-tick-capped pull blended
        // on top of the player's own WASD swing.
        const acc::minigame::MagnetParams mp{
            kMagnetGainFar, kMagnetGainNear, kMagnetMaxStepDeg};
        gain  = acc::minigame::MagnetGain(1.0f - angle / kMagnetEngageDeg, mp);
        newAz = acc::minigame::MagnetStep(offAz, errAzOff, gain, mp);
        newEl = acc::minigame::MagnetStep(offEl, errElOff, gain, mp);
    }
    // Keep the write inside what KeepInTunnel will allow, so the value
    // we write is the value the gun takes. A free azimuth wraps 0..360;
    // a bounded axis clamps to the tunnel. Before the limits are read
    // the azimuth wraps as it always has and elevation is left alone.
    if (azBounded) {
        if (newAz < arena.az_lo) newAz = arena.az_lo;
        if (newAz > arena.az_hi) newAz = arena.az_hi;
    } else {
        while (newAz >= 360.0f) newAz -= 360.0f;
        while (newAz <   0.0f)  newAz += 360.0f;
    }
    if (elBounded) {
        if (newEl < arena.el_lo) newEl = arena.el_lo;
        if (newEl > arena.el_hi) newEl = arena.el_hi;
    }
    WriteOffset(newAz, newEl);
    return true;
}

// ---- Full-autoaim makeability retarget --------------------------------
// Hold the lock while the shot is makeable. Once it's been unmakeable past
// the debounce (and no recent manual Q/E to honour), hand the lock to the
// nearest fighter — range dominates crossingSpeed·range, so nearest is the
// most makeable — instead of chasing this one through its unhittable orbit.
// Silent (AnnounceSelectedTarget's locator ping only) so it never spams
// speech. Assisted mode never auto-retargets (fullAuto gate).
void AutoaimRetarget(bool justLandedHit, bool makeable, float leadT,
                     float crossingSpeed, float sd, const int occSlot[],
                     const float occDist[], const Vector occPos[],
                     const TargetInfo occInfo[], int occCount,
                     const Vector& listener) {
    const ULONGLONG nowMs  = GetTickCount64();
    const bool      graced = (nowMs - g_state.last_manual_cycle_ms) < kManualGraceMs;

    // (1) Switch-on-hit: we just damaged the locked fighter, so it's now in
    // its invuln window — spread to the nearest OTHER fighter that isn't also
    // mid-invuln (census is nearest-first; fall back to nearest other if all
    // are cooling). Nearest = shortest flight, so the freshly-baselined lead
    // is reliable. Bypasses the commit cooldown by design.
    bool switched = false;
    if (justLandedHit && !graced && occCount > 1) {
        int pick = -1, fallback = -1;
        for (int a = 0; a < occCount; ++a) {
            if (occSlot[a] == g_state.selected_slot) continue;
            if (fallback < 0) fallback = a;
            const ULONGLONG lh = g_state.last_hit_ms[occSlot[a]];
            if (nowMs - lh >= kInvulnWindowMs) { pick = a; break; }
        }
        if (pick < 0) pick = fallback;
        if (pick >= 0) {
            acclog::Write("TurretAssist",
                          "spread (on hit) slot=%d->%d vulnerable=%d",
                          g_state.selected_slot, occSlot[pick], (pick != fallback) ? 1 : 0);
            AnnounceSelectedTarget(pick, occSlot, occDist, occPos, occInfo, occCount,
                                   listener, /*speak=*/false);
            g_state.unmakeable_since_ms = 0;
            g_state.last_retarget_ms    = nowMs;
            switched = true;
        }
    }

    // (2) Unmakeable fallback: stuck on a target we can't hit at all (too far)
    // — debounced + cooldown'd switch to nearest. Skipped if we just spread.
    if (!switched) {
        if (makeable) {
            g_state.unmakeable_since_ms = 0;
        } else {
            if (g_state.unmakeable_since_ms == 0) g_state.unmakeable_since_ms = nowMs;
            const bool cooling   = (nowMs - g_state.last_retarget_ms)    < kRetargetCooldownMs;
            const bool debounced = (nowMs - g_state.unmakeable_since_ms) >= kRetargetDebounceMs;
            if (!graced && !cooling && debounced && occSlot[0] != g_state.selected_slot) {
                acclog::Write("TurretAssist",
                              "retarget (unmakeable) slot=%d->%d leadT=%.2f "
                              "crossing=%.0f range=%.0f",
                              g_state.selected_slot, occSlot[0], leadT, crossingSpeed, sd);
                AnnounceSelectedTarget(0, occSlot, occDist, occPos, occInfo, occCount,
                                       listener, /*speak=*/false);
                g_state.unmakeable_since_ms = 0;
                g_state.last_retarget_ms    = nowMs;
            }
        }
    }
}

}  // namespace acc::turret_game

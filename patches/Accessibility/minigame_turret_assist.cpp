// The aim assist: the fighter's motion estimate, the predictive lead, the
// offset-sign calibration, steering the gun, and full-autoaim's retargeting.

#include "minigame_turret_internal.h"

#include "log.h"

namespace acc::turret_game {

namespace {

// ----- Predictive lead (intercept aiming) -----
// The cue aims at where the fighter WILL BE when a bolt arrives, not where it
// is now — the audio analog of a sighted player pulling ahead of the target.
// The bolt speed and lifespan are read off the player's gun bank at entry
// (Arena) — the area's own MiniGame -> Player -> Gun_Banks -> Bullet values:
// 300 u/s for 3 s on K1's M12ab, 600 u/s on K2's Dxun and Onderon. With K1's
// (slow-ish) bolt the measured critBoltSpeed (149 close / 753 mid / 1366 far)
// means lead is unnecessary close but ESSENTIAL at mid/far, where the
// ~13.5deg lead exceeds the hitbox window — so leading converts mid-range
// from a systematic miss into a hittable shot. We solve the intercept in the
// LISTENER frame (relative position + relative velocity) so the result is
// directly "where to point the turret" and ship motion folds in automatically.
// The intercept is clamped to the bolt lifespan; a hitscan area gets no lead.
constexpr float kVelEmaAlpha    = 0.25f;   // velocity smoothing (per-tick Δpos)
// Range-gate the lead: it is UNNECESSARY when the fighter would stay inside its
// own hitbox during the bolt's flight (the centre-aim margin already covers
// that drift), and applying it there only pulls the loud spot off the ship and
// adds velocity-jitter — which measurably hurt the close-range aim that wins
// the game. So ramp the lead in by how far the fighter drifts during flight
// relative to its radius: none below kLeadGateLowFrac·radius, full by
// kLeadGateHighFrac·radius. This auto-adapts to each fighter's actual speed.
constexpr float kLeadGateLowFrac  = 0.75f;  // start leading at 0.75·radius drift
constexpr float kLeadGateHighFrac = 1.5f;   // full lead by 1.5·radius drift

// ----- Curve-aware lead (centripetal turn model) -----
// The fighters hold ~constant speed and only TURN (relVel sat at ~74 u/s at every
// range in the logs), so their acceleration is essentially centripetal. We
// estimate it as the PERPENDICULAR component of d(vel_ema)/dt — discarding the
// along-track part enforces the constant-speed prior and kills most of the noise
// — smooth it harder than velocity (a second derivative is noisier), and fold a
// 0.5*a*t^2 term into the intercept via a fixed-point flight-time solve. Heavy
// smoothing is safe because the curve term is weighted by t^2, which is tiny on
// the close fast passes (short flight, noisiest estimate) and only grows at
// mid-range where the turn is steady and easy to track. The correction is
// leadFrac-gated, flight-time-capped, and clamped to never exceed the linear
// lead, so it sharpens mid-range aim without destabilising close or far shots.
constexpr float kAccelEmaAlpha       = 0.12f;  // centripetal-accel smoothing (< vel alpha)
constexpr int   kAccelSettleSamples  = 5;      // ticks before the turn estimate is trusted
constexpr float kMaxCentripetalAccel = 300.0f; // u/s^2 sanity clamp on the estimate
constexpr float kAccelLeadCapSeconds = 1.0f;   // cap on the flight-time used in 0.5*a*t^2

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

// ---- Lead: aim at the intercept point. Solve |P + v·t| = boltSpeed·t for
// the smallest positive t (a·t² + b·t + c = 0), then I = P + v·t. Falls
// back to the current position when velocity isn't established yet or no
// valid intercept exists. Clamped to the bolt lifespan. A hitscan area's
// hits don't fly, so it aims at the target itself. ----------------------
LeadSolution SolveLead(const Vector& P, const Vector& v, float vmag, float radius,
                       const Arena& arena) {
    const float  boltSpeed  = arena.bolt_speed;
    const float  boltLife   = arena.bolt_life;
    Vector aimRel = P;           // listener-relative point the cue aims at
    Vector aimRelLin = P;        // linear-only baseline (LeadCheck diagnostic)
    float  leadT = 0.0f, leadDist = 0.0f, leadFrac = 0.0f;
    if (g_state.have_vel && !arena.hitscan) {
        const float a  = (v.x * v.x + v.y * v.y + v.z * v.z) - boltSpeed * boltSpeed;
        const float bq = 2.0f * (P.x * v.x + P.y * v.y + P.z * v.z);
        const float cq = (P.x * P.x + P.y * P.y + P.z * P.z);
        float t = -1.0f;
        if (std::fabs(a) < 1e-3f) {
            if (std::fabs(bq) > 1e-6f) t = -cq / bq;
        } else {
            const float disc = bq * bq - 4.0f * a * cq;
            if (disc >= 0.0f) {
                const float sq = std::sqrt(disc);
                const float t1 = (-bq + sq) / (2.0f * a);
                const float t2 = (-bq - sq) / (2.0f * a);
                if (t1 > 0.0f)                       t = t1;
                if (t2 > 0.0f && (t < 0.0f || t2 < t)) t = t2;
            }
        }
        if (t > 0.0f && t <= boltLife) {
            leadT = t;
            const float fullLead = vmag * t;  // drift during flight
            // Range-gate: ramp lead in by drift relative to the hitbox radius,
            // so close shots (drift < ~0.75·radius) keep the pure ship aim.
            const float lo = kLeadGateLowFrac  * radius;
            const float hi = kLeadGateHighFrac * radius;
            if (radius > 0.0f && hi > lo) {
                leadFrac = (fullLead - lo) / (hi - lo);
                if (leadFrac < 0.0f)      leadFrac = 0.0f;
                else if (leadFrac > 1.0f) leadFrac = 1.0f;
            } else {
                leadFrac = 1.0f;  // no radius this tick — lead fully
            }
            aimRel.x = P.x + v.x * t * leadFrac;
            aimRel.y = P.y + v.y * t * leadFrac;
            aimRel.z = P.z + v.z * t * leadFrac;
            leadDist = fullLead * leadFrac;  // applied lead
            aimRelLin = aimRel;              // capture the linear baseline

            // Curve-aware refinement (constant centripetal accel). A few
            // flight-time fixed-point iterations layer a 0.5·a·t² turn term onto
            // the linear lead. The accel part is leadFrac-gated, flight-time-
            // capped, and clamped to never exceed the linear lead — sharpening
            // mid-range aim without destabilising close or far shots. This is the
            // SHARED aim point: cue, magnetism, and autoaim all read aimRel.
            if (g_state.have_accel) {
                const Vector ac = g_state.accel_ema;
                float tc = t;
                Vector pred = aimRel;
                float corrMag = leadDist;
                for (int it = 0; it < 3; ++it) {
                    const float te = (tc < kAccelLeadCapSeconds) ? tc : kAccelLeadCapSeconds;
                    const float half = 0.5f * te * te * leadFrac;
                    Vector linPart = {v.x * tc * leadFrac, v.y * tc * leadFrac, v.z * tc * leadFrac};
                    Vector accPart = {ac.x * half, ac.y * half, ac.z * half};
                    const float lpm = std::sqrt(linPart.x * linPart.x + linPart.y * linPart.y + linPart.z * linPart.z);
                    const float apm = std::sqrt(accPart.x * accPart.x + accPart.y * accPart.y + accPart.z * accPart.z);
                    if (apm > lpm && apm > 1e-4f) {  // curve part can't exceed linear lead
                        const float s = lpm / apm;
                        accPart.x *= s; accPart.y *= s; accPart.z *= s;
                    }
                    const Vector corr = {linPart.x + accPart.x, linPart.y + accPart.y, linPart.z + accPart.z};
                    pred = {P.x + corr.x, P.y + corr.y, P.z + corr.z};
                    corrMag = std::sqrt(corr.x * corr.x + corr.y * corr.y + corr.z * corr.z);
                    const float pd = std::sqrt(pred.x * pred.x + pred.y * pred.y + pred.z * pred.z);
                    tc = pd / boltSpeed;
                    if (tc > boltLife) tc = boltLife;
                    else if (tc < 0.0f)       tc = 0.0f;
                }
                aimRel   = pred;
                leadT    = tc;
                leadDist = corrMag;
            }
        }
    }
    return {aimRel, aimRelLin, leadT, leadDist, leadFrac};
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
        // Magnetism (shared facility, minigame_aim.h): proximity-ramped
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
                              "retarget (unmakeable) slot=%d->%d leadT=%.2f crossing=%.0f range=%.0f",
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

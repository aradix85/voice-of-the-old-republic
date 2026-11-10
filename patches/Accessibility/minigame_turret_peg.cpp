// DriveSelectedPeg: per tick, the audio peg that guides the aim onto the
// selected target (pitch and pan, lock and fire cues, lead, aim assist).

#include "minigame_turret_internal.h"

#include "audio_bus.h"        // PlayCue3D — one-shot lock-confirmation ping
#include "menus_modsettings.h" // TurretAutoAim easy-mode toggle
#include "log.h"

namespace acc::turret_game {

namespace {

// ----- Behind steering (rear-arc pinned pan) -----
// In the rear hemisphere the target can't masquerade as a front one, so the
// ticks (behind is always off-target) PIN hard to the shorter-swing side: pan
// left = swing left, pan right = swing right. The side is latched through a
// near-zero (dead-behind) lateral so it doesn't flip-flop. Hysteretic gate
// (kBehindEnterDeg/kBehindExitDeg) so a target orbiting the ~90° boundary
// doesn't flap. The elevation pitch glide applies to the solid tone (set
// unconditionally below) once the target is back in the forward cone.
constexpr float kBehindPanDist = 9.0f;  // pinned tick distance while behind

// ----- Elevation channel (peg playback PITCH = vertical aim error) -----
// Stereo pan carries azimuth (left/right) but nothing about elevation, so the
// peg's loudness alone left the up/down axis blind — the dominant miss at
// range (QC: vertically on-target only ~15% of frames; targets span -7°..+73°
// elevation against a fixed aim). We add the orthogonal channel as PITCH: the
// peg plays at its natural rate while the aim is within the hitbox's elevation
// tolerance ("sounds normal" = on the vertical line, a shot connects), and
// glides off-pitch outside it — HIGHER when the target is above the aim (press
// W) and LOWER when below (press S). The dead band is the hitbox subtend (the
// same range-scaled tolerance the azimuth/loudness logic already uses), so the
// "normal pitch" landmark means "elevation is inside the hitbox" at any range:
// wide and forgiving up close, tight far out.
constexpr float kElevDegPerOctave = 25.0f;  // vertical error beyond band per octave
constexpr float kElevMaxOctaves   = 1.0f;   // clamp the glide to +/- 1 octave

// ----- "Fire now" lock cue (one-shot, on entering the hitbox) -----
// The single most useful signal for a blind gunner: a distinct one-shot the
// instant the COMBINED aim (azimuth via pan + elevation via pitch) crosses
// INTO the hitbox cone (onTarget false->true) — "both axes aligned, shoot".
// It collapses the two-axis judgement into one reaction, and self-gates by the
// hitbox geometry: it fires readily up close (wide cone) and almost never far
// (3° cone), which correctly signals that far fighters aren't worth firing at.
// One-shot, not continuous — the player is rarely on a fast target long enough
// to ride a sustained tone. Played toward the fighter so it pans correctly.
constexpr const char* kFireCueResref = "c_drdastro_hit2";

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

// Drive the continuous peg tone on the selected fighter. Loudness (via source
// distance) encodes the aim error to THAT fighter — faint off-aim, peaks on
// target. When the selected fighter leaves the census (destroyed / out of
// range), AUTO-ADVANCE to the nearest remaining fighter and lock it; if none
// remain, announce "target lost" and fall silent.
void DriveSelectedPeg(const int occSlot[], const float occDist[],
                      const Vector occPos[], const TargetInfo occInfo[],
                      int occCount, const Vector& aimDir, bool haveAim,
                      const Vector& listener, bool selectedAliveInPool) {
    int idx = CensusIndexOf(occSlot, occCount, g_state.selected_slot);
    // True only when the locked fighter was already in the census on entry
    // (not a fresh re-pick this tick) — gates the HP/hit read so the info we
    // log belongs to the fighter we were tracking.
    const bool trackingExisting = (idx >= 0);

    if (idx < 0) {
        // The locked fighter is not in the census this tick.
        if (g_state.selected_slot >= 0) {
            // Distinguish a KILL (the fighter left the MGO pool entirely —
            // AsEnemy no longer resolves it) from a fighter that is still
            // alive but merely dropped out of the census this tick (flew
            // beyond cue range / off-screen, or a one-tick position-read miss).
            // selectedAliveInPool is set by TickFighterCues' Pass-1 walk
            // regardless of range.
            const bool killed = !selectedAliveInPool;

            if (!killed) {
                // Alive, just out of view. HOLD the lock and fall silent —
                // do NOT re-pick and do NOT announce. The player keeps the
                // same target and can wait for it to come back or manually
                // retarget with Q/E. (Re-picking here jolted the lock, and a
                // transient read miss could trip it spuriously; re-pick now
                // happens only on round start and on a kill.)
                if (g_state.peg_cue.IsActive()) {
                    g_state.peg_cue.Stop();
                    acclog::Write("Turret",
                                  "peg off (out of view) — holding lock slot=%d",
                                  g_state.selected_slot);
                }
                g_state.selected_on_target = false;  // re-arm fire-now on return
                g_state.selected_behind    = false;  // re-evaluate gate on return
                // (no assist write this tick — we hold the lock but don't steer)
                return;
            }

            // KILL: the selected fighter left the pool. The global per-slot
            // down-detector in TickFighterCues already announced "Fighter N
            // destroyed", so here we only transition the lock — stop the cue
            // and auto-advance to the nearest remaining fighter (silently).
            if (g_state.peg_cue.IsActive()) {
                g_state.peg_cue.Stop();
                acclog::Write("Turret", "peg off (selected destroyed) slot=%d",
                              g_state.selected_slot);
            }

            if (occCount > 0) {
                AnnounceSelectedTarget(0, occSlot, occDist, occPos, occInfo, occCount,
                                       listener, /*speak=*/false);
                idx = 0;
            } else {
                g_state.selected_slot = -1;
                acclog::Write("Turret", "no fighters remain (after kill)");
                return;
            }
        } else {
            // Nothing selected yet (game start / all clear). Auto-pick the
            // nearest fighter the instant one is in range, so the player
            // begins the round already locked without pressing Q/E — same
            // nearest-first rule as the lose-target auto-advance above.
            if (occCount > 0) {
                AnnounceSelectedTarget(0, occSlot, occDist, occPos, occInfo, occCount,
                                       listener);
                idx = 0;
            } else {
                return;  // no fighters in range yet — wait
            }
        }
    }

    if (!haveAim) {
        return;  // hold the cue; can't read the crosshair this tick
    }

    constexpr float kRad2Deg = 57.2957795f;
    auto wrap180 = [](float d) {
        while (d > 180.0f)  d -= 360.0f;
        while (d < -180.0f) d += 360.0f;
        return d;
    };
    auto clamp1 = [](float v) { return v > 1.0f ? 1.0f : (v < -1.0f ? -1.0f : v); };

    const Vector& sp = occPos[idx];
    const float   sd = occDist[idx];

    // Range-window cue: punctuate the locked fighter crossing INTO killable
    // range (enter) and back OUT of it (leave). Fires independent of the
    // front-cone gate — it's about distance, not bearing. Same sample for both
    // for now; the crosshair tone swelling (enter) vs fading (leave) is the
    // current disambiguator. Played at a compressed, always-audible point in
    // the fighter's direction so it pans toward it (the lock-locator trick).
    {
        const Vector& ve = g_state.vel_ema;
        const float measured = g_state.have_vel
            ? std::sqrt(ve.x * ve.x + ve.y * ve.y + ve.z * ve.z) : 0.0f;
        const float killable = KillableRangeM(occInfo[idx].radius,
                                              TargetSpeed(&occInfo[idx], measured));
        const int wasInRange = g_state.selected_in_range;          // -1/0/1
        const int nowInRange = (sd > 0.0f && sd <= killable) ? 1 : 0;
        if (wasInRange >= 0 && nowInRange != wasInRange) {
            const Vector pos = PlayRangeCueAt(sp, sd, listener);
            if (!nowInRange) {
                // LEAVE: schedule a 2nd tap so a double-tap unambiguously means
                // "window closed / reacquire", vs the single tap for ENTER.
                g_state.leave_tap_due_ms = GetTickCount64() + kLeaveTapGapMs;
                g_state.leave_tap_pos    = pos;
            }
            acclog::Write("Turret", "range cue: %s slot=%d dist=%.0fm",
                          nowInRange ? "ENTER" : "LEAVE",
                          g_state.selected_slot, sd);
        }
        g_state.selected_in_range = nowInRange;
    }

    // Fighter position relative to the listener (gun). We solve the whole lead
    // in this frame so the answer is "where to point the turret".
    const Vector P = {sp.x - listener.x, sp.y - listener.y, sp.z - listener.z};
    // |P|. Equal to the census distance sd when `listener` is the camera; a
    // hitscan area passes the barrel instead (see TickFighterCues).
    const float  pLen = std::sqrt(P.x * P.x + P.y * P.y + P.z * P.z);

    // ---- Smoothed relative velocity (Δ relative-pos / Δt, EMA). ------------
    const ULONGLONG now = GetTickCount64();
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
    const Vector v = g_state.vel_ema;
    const float  vmag = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);

    // ---- Combat fields of the selected target (census snapshot). ----------
    int   hp = -1, maxHp = -1;
    float radius = 0.0f, engineSpeed = 0.0f;
    if (trackingExisting) {
        const TargetInfo& ti = occInfo[idx];
        hp          = ti.hp;
        maxHp       = ti.max_hp;
        radius      = ti.radius;
        engineSpeed = ti.engine_speed;
    }

    // ---- Lead: aim at the intercept point. Solve |P + v·t| = boltSpeed·t for
    // the smallest positive t (a·t² + b·t + c = 0), then I = P + v·t. Falls
    // back to the current position when velocity isn't established yet or no
    // valid intercept exists. Clamped to the bolt lifespan. A hitscan area's
    // hits don't fly, so it aims at the target itself. ----------------------
    const Arena& arena      = g_state.arena;
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

    // Aim error to the intercept point (this drives the cue); also the error to
    // the CURRENT fighter (no-lead) for diagnostic comparison.
    const float aimTgtDist =
        std::sqrt(aimRel.x * aimRel.x + aimRel.y * aimRel.y + aimRel.z * aimRel.z);
    Vector tdir = {0.0f, 0.0f, 0.0f};
    float  angle = 0.0f;
    if (aimTgtDist > 0.0f) {
        tdir.x = aimRel.x / aimTgtDist;
        tdir.y = aimRel.y / aimTgtDist;
        tdir.z = aimRel.z / aimTgtDist;
        angle = AngleBetweenDeg(aimDir, tdir);
    }
    float angleNow = 0.0f;
    if (pLen > 0.0f) {
        const Vector nd = {P.x / pLen, P.y / pLen, P.z / pLen};
        angleNow = AngleBetweenDeg(aimDir, nd);
    }

    // Hitbox half-angle at the INTERCEPT distance — "on target" = a bolt fired
    // now lands in the sphere when it arrives.
    const float subtendDeg =
        (radius > 0.0f && aimTgtDist > 0.0f) ? std::atan(radius / aimTgtDist) * kRad2Deg : 0.0f;
    const float onTargetAngle =
        (subtendDeg > 0.0f) ? subtendDeg : kFallbackOnTargetDeg;
    const bool  onTarget = (angle <= onTargetAngle);

    // ---- Makeability (full-autoaim only) -----------------------------------
    // The lead cancels the crossing drift, so the limiter is the extrapolation
    // horizon: makeable while the bolt's flight time is short enough that the
    // prediction stays reliable. crossingSpeed is kept for the diagnostic only.
    const bool fullAuto = acc::menus::modsettings::GetToggle(
                              acc::menus::modsettings::Option::TurretAutoAim);
    float crossingSpeed = 0.0f;
    if (pLen > 1e-3f) {
        const Vector rhat = {P.x / pLen, P.y / pLen, P.z / pLen};
        const float  vrad = v.x * rhat.x + v.y * rhat.y + v.z * rhat.z;
        const Vector vtan = {v.x - vrad * rhat.x, v.y - vrad * rhat.y, v.z - vrad * rhat.z};
        crossingSpeed = std::sqrt(vtan.x * vtan.x + vtan.y * vtan.y + vtan.z * vtan.z);
    }
    // A hitscan area has no flight to extrapolate, so every target is makeable.
    const float makeHorizon = MakeHorizonSec(radius, TargetSpeed(
        trackingExisting ? &occInfo[idx] : nullptr, vmag));
    const bool  makeable = arena.hitscan ||
                           (g_state.have_vel && leadT > 0.0f && leadT <= makeHorizon);
    // In full-autoaim the cue must stay honest: the solid "fire" tone fires only
    // when the shot actually connects, so it can't lie while the gun tracks a
    // target that's crossing too fast to hit. Assisted mode is untouched — there
    // the tone reflects the player's own aim, which is already honest.
    const bool  onTargetCue = onTarget && (!fullAuto || makeable);

    // "Fire now": one-shot the instant the combined aim crosses INTO the hitbox
    // (false->true). Played at a compressed, always-audible point toward the
    // fighter so it pans correctly. selected_on_target latches so it fires once
    // per entry, not every tick you hold the cone.
    if (onTargetCue && !g_state.selected_on_target) {
        Vector firePos = sp;
        if (sd > 0.0f) {
            const float fk = kPegMinDist / sd;
            firePos.x = listener.x + (sp.x - listener.x) * fk;
            firePos.y = listener.y + (sp.y - listener.y) * fk;
            firePos.z = listener.z + (sp.z - listener.z) * fk;
        }
        acc::audio::PlayCue3D(kFireCueResref, firePos);
        acclog::Write("Turret", "fire-now slot=%d angle=%.1f subtend=%.1f dist=%.0fm",
                      g_state.selected_slot, angle, subtendDeg, sd);
    }
    g_state.selected_on_target = onTargetCue;

    // Loudness: rise gently from the hitbox edge (kPegEdgeDist) to a peak at
    // dead-centre (kPegMinDist); beyond the edge, fade to faint over
    // kPegRampDeg. Continuous at the boundary.
    float srcDist;
    if (onTargetCue) {
        const float u = (onTargetAngle > 0.0f) ? angle / onTargetAngle : 0.0f;
        srcDist = kPegMinDist + u * (kPegEdgeDist - kPegMinDist);
    } else {
        float t = (angle - onTargetAngle) / kPegRampDeg;
        if (t > 1.0f) t = 1.0f;
        srcDist = kPegEdgeDist + t * (kPegMaxDist - kPegEdgeDist);
    }
    const float kk = (aimTgtDist > 0.0f) ? srcDist / aimTgtDist : 1.0f;
    const Vector cuePos = {
        listener.x + aimRel.x * kk,
        listener.y + aimRel.y * kk,
        listener.z + aimRel.z * kk,
    };

    // Behind-gate (hysteretic): a target orbiting the ~90° boundary mustn't flap.
    // Cross INTO behind above kBehindEnterDeg, back to front below kBehindExitDeg.
    // Behind is always off-target (onTargetAngle is small), so it always ticks —
    // pinned to the shorter-swing side.
    const bool behind = g_state.selected_behind ? (angle >= kBehindExitDeg)
                                                : (angle > kBehindEnterDeg);
    const bool behindEntered = behind && !g_state.selected_behind;
    g_state.selected_behind = behind;

    // Where the off-target pulse plays. Forward: toward the aim/intercept at a
    // fixed audible distance, so pan points the way and volume stays steady
    // (rate, not loudness, carries closeness). Behind: pinned to the shorter-
    // swing side (2D cross product of aim x target gives the side; latched
    // through a near-zero dead-behind lateral so it doesn't flip-flop).
    Vector tickPos;
    bool   swingLeft = g_state.last_behind_swing_left;
    if (behind) {
        const float cross = aimDir.x * tdir.y - aimDir.y * tdir.x;
        if (cross > 0.05f)       swingLeft = true;
        else if (cross < -0.05f) swingLeft = false;
        g_state.last_behind_swing_left = swingLeft;
        float sx = swingLeft ? -aimDir.y : aimDir.y;
        float sy = swingLeft ?  aimDir.x : -aimDir.x;
        const float sm = std::sqrt(sx * sx + sy * sy);
        if (sm > 1e-4f) { sx /= sm; sy /= sm; }
        tickPos = { listener.x + sx * kBehindPanDist,
                    listener.y + sy * kBehindPanDist,
                    listener.z };
    } else {
        tickPos = { listener.x + tdir.x * kTickDist,
                    listener.y + tdir.y * kTickDist,
                    listener.z + tdir.z * kTickDist };
    }

    if (onTargetCue && !behind) {
        // SOLID: continuous tone at the loudness-ramped position, which still
        // swells toward dead-centre within the cone (the bolt-margin reward).
        // The elevation pitch glide applies below. This is the unambiguous
        // "a shot connects — fire" signal, at any range.
        if (g_state.peg_cue.IsActive()) {
            g_state.peg_cue.UpdatePosition(cuePos);
        } else if (g_state.peg_cue.Start(kLockCueResref, cuePos,
                       /*looping=*/true, /*spatial=*/true, /*priorityGroup=*/-1,
                       /*volumeByte=*/-1, /*maxVolDist=*/kPegMinDist,
                       /*minVolDist=*/kPegMaxDist)) {
            acclog::Write("Turret", "peg SOLID (on target) slot=%d angle=%.1f subtend=%.1f",
                          g_state.selected_slot, angle, subtendDeg);
        }
    } else {
        // TICKING: stop the solid loop; pulse a one-shot whose interval shrinks
        // as the aim approaches the cone (fast just outside, slow far off/behind).
        if (g_state.peg_cue.IsActive()) {
            g_state.peg_cue.Stop();
            acclog::Write("Turret", "peg -> tick mode slot=%d angle=%.1f%s",
                          g_state.selected_slot, angle, behind ? " (behind)" : "");
        } else if (behindEntered) {
            acclog::Write("Turret", "tick behind: pin %s slot=%d angle=%.0f",
                          swingLeft ? "LEFT" : "RIGHT", g_state.selected_slot, angle);
        }
        float frac = (angle - onTargetAngle) / kTickRampDeg;
        if (frac < 0.0f) frac = 0.0f; else if (frac > 1.0f) frac = 1.0f;
        const ULONGLONG interval = kTickFastMs +
            static_cast<ULONGLONG>(frac * (kTickSlowMs - kTickFastMs));
        const ULONGLONG nowT = GetTickCount64();
        if (nowT - g_state.last_tick_ms >= interval) {
            g_state.last_tick_ms = nowT;
            acc::audio::PlayCue3D(kTickCueResref, tickPos);
        }
    }

    const float lateral = std::sin(angle * 0.01745329f) * aimTgtDist;

    // Per-axis error to the INTERCEPT direction (the cue's target).
    const float aimAz = std::atan2(aimDir.x, aimDir.y) * kRad2Deg;
    const float aimEl = std::asin(clamp1(aimDir.z))   * kRad2Deg;
    const float tgtAz = std::atan2(tdir.x,   tdir.y)  * kRad2Deg;
    const float tgtEl = std::asin(clamp1(tdir.z))     * kRad2Deg;
    const float errAz = wrap180(aimAz - tgtAz);
    const float errEl = aimEl - tgtEl;

    // ---- Aim-assist: STEER THE GUN by writing offset (+0x1c4). --------------
    // Reduce the world aim error (errAz/errEl, crosshair -> intercept) by nudging
    // offset.z/.x toward the target. The offset->world sign is passively
    // calibrated (below) so we never hard-code the engine's rotation convention;
    // the calibration also auto-corrects an inverted forward axis in ReadAimLine.
    //   * Full-autoaim toggle ON: own the gun, snap onto the locked intercept.
    //   * Assist (default magnetism): only inside kAssistZoneDeg, gentle, and
    //     yield to a sustained manual WASD swing.
    float offAz = 0.0f, offEl = 0.0f;
    const bool haveOffset = ReadOffset(offAz, offEl);

    // Passive sign calibration: learn the sign of d(worldAim)/d(offset) per axis
    // from frame-to-frame motion (player swing or our own writes). Voted for
    // stability; target-independent (uses aim motion, not the target).
    if (haveOffset && haveAim) {
        if (g_state.calib_have) {
            const float dOffAz = wrap180(offAz - g_state.calib_last_off_az);
            const float dAimAz = wrap180(aimAz - g_state.calib_last_aim_az);
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
    const ULONGLONG nowMsA = GetTickCount64();
    bool  steer = haveOffset && haveAim;
    float gain  = 0.0f;
    if (steer && !fullAuto && angle > kMagnetEngageDeg)
        steer = false;  // target too far off-aim (or behind) — you swing onto it
    if (steer) {
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
    }

    // Throttled assist/calibration log (~1/s).
    if (aimTgtDist > 0.0f && nowMsA - g_state.assist_last_log_ms >= 1000) {
        g_state.assist_last_log_ms = nowMsA;
        acclog::Write("TurretCue",
                      "slot=%d angle=%.1f errAz=%.1f errEl=%.1f deadband=%.1f "
                      "dist=%.0f off(az=%.1f el=%.1f) aim(az=%.1f el=%.1f) "
                      "calibAz=%d calibEl=%d steer=%d gain=%.2f auto=%d",
                      g_state.selected_slot, angle, errAz, errEl, onTargetAngle,
                      sd, offAz, offEl, aimAz, aimEl,
                      g_state.calib_sign_az, g_state.calib_sign_el,
                      steer ? 1 : 0, gain, fullAuto ? 1 : 0);
    }

    // ---- Elevation channel: drive the peg's PITCH from the vertical error. --
    // elevErr > 0 -> target ABOVE the aim (glide pitch up, player presses W);
    // < 0 -> below (pitch down, S). Inside the hitbox's elevation tolerance
    // (dead band = subtend) the peg holds its natural pitch — "sounds normal"
    // == on the vertical line. No-op when the peg is silenced (behind-gate) or
    // the 3D voice isn't up yet. The loudness/pan channels are unchanged, so
    // this adds the up/down axis without touching the existing azimuth feel.
    const float elevErr  = -errEl;  // = tgtEl - aimEl
    const float elevBand = (subtendDeg > 0.0f) ? subtendDeg : kFallbackOnTargetDeg;
    float elevBeyond = 0.0f;
    if (elevErr > elevBand)       elevBeyond = elevErr - elevBand;
    else if (elevErr < -elevBand) elevBeyond = elevErr + elevBand;
    float elevOct = elevBeyond / kElevDegPerOctave;
    if (elevOct > kElevMaxOctaves)       elevOct = kElevMaxOctaves;
    else if (elevOct < -kElevMaxOctaves) elevOct = -kElevMaxOctaves;
    const float pitchMult = std::pow(2.0f, elevOct);
    g_state.peg_cue.SetPitchMultiplier(pitchMult);

    // Real engine-scored hit (hp drop). Engine plays its own mgs_sith_hit, so
    // this is diagnostic only. errAngle here is to the intercept; lead-correct
    // shots should now show small errAngle at the hit frame (validates lead).
    bool justLandedHit = false;
    if (trackingExisting && hp >= 0) {
        if (g_state.last_selected_hp >= 0 && hp < g_state.last_selected_hp) {
            acclog::Write("TurretHit",
                          "slot=%d number=%d hp=%d->%d errAngle=%.1f errNow=%.1f "
                          "errAz=%.1f errEl=%.1f leadT=%.2f leadDist=%.0f "
                          "dist=%.0f radius=%.1f subtendDeg=%.1f",
                          g_state.selected_slot, NumberForSlot(g_state.selected_slot),
                          g_state.last_selected_hp, hp, angle, angleNow, errAz, errEl,
                          leadT, leadDist, sd, radius, subtendDeg);
            // We damaged it — it's now invulnerable for a beat. Mark it and spread.
            if (g_state.selected_slot >= 0 && g_state.selected_slot < kMgoArraySlotCount)
                g_state.last_hit_ms[g_state.selected_slot] = now;
            justLandedHit = true;
        }
        g_state.last_selected_hp = hp;
    }

    // Lead/velocity diagnostic: relative speed, the intercept lead, and
    // critBoltSpeed (= |v|·dist/radius — the bolt speed below which lead matters).
    acclog::Write("TurretVel",
                  "slot=%d relVelMS=%.1f engineSpeed=%.1f dist=%.0f leadT=%.2f "
                  "leadDist=%.0f leadFrac=%.2f aimDist=%.0f critBoltSpeed=%.0f "
                  "crossing=%.1f make=%.0f makeable=%d",
                  g_state.selected_slot, vmag, engineSpeed, sd, leadT, leadDist,
                  leadFrac, aimTgtDist, (radius > 0.0f) ? vmag * sd / radius : 0.0f,
                  crossingSpeed, crossingSpeed * sd, makeable ? 1 : 0);

    // LeadCheck: drain any matured predictions against the fighter's real
    // position now, then (throttled) enqueue this tick's linear & curve
    // intercepts in world space for a later maturity comparison.
    DrainLeadProbes(g_state.selected_slot, sp, now);
    if (g_state.have_vel && leadT > 0.0f && now - g_state.lead_probe_push_ms >= 200) {
        g_state.lead_probe_push_ms = now;
        const Vector wLin   = {listener.x + aimRelLin.x, listener.y + aimRelLin.y, listener.z + aimRelLin.z};
        const Vector wCurve = {listener.x + aimRel.x,    listener.y + aimRel.y,    listener.z + aimRel.z};
        PushLeadProbe(g_state.selected_slot, wLin, wCurve,
                      now + static_cast<ULONGLONG>(leadT * 1000.0f));
    }

    // QC accumulation (aim quality at the intercept — the operative on-target).
    {
        // Bands scale with the killable range (K1: 130 m -> 100 / 250 m).
        const float kr = KillableRangeM(radius, TargetSpeed(
            trackingExisting ? &occInfo[idx] : nullptr, vmag));
        const int band = (sd < kr * (100.0f / 130.0f)) ? 0
                       : (sd < kr * (250.0f / 130.0f) ? 1 : 2);
        ++g_state.qc_frames;
        ++g_state.qc_n[band];
        if (onTarget) { ++g_state.qc_on_total; ++g_state.qc_on[band]; }
        if (angle < g_state.qc_min_err) g_state.qc_min_err = angle;
        g_state.qc_sum_err += angle;
    }

    acclog::Write("TurretAim",
                  "selected slot=%d errAngle=%.1f errNow=%.1f errAz=%.1f errEl=%.1f "
                  "aim(az=%.1f el=%.1f) tgt(az=%.1f el=%.1f) leadT=%.2f leadDist=%.0f "
                  "lateralMiss=%.0fm dist=%.0f aimDist=%.0f srcDist=%.1f hp=%d/%d "
                  "radius=%.1f subtendDeg=%.1f onTarget=%d elevErr=%.1f elevBand=%.1f "
                  "pitchMult=%.2f",
                  g_state.selected_slot, angle, angleNow, errAz, errEl,
                  aimAz, aimEl, tgtAz, tgtEl, leadT, leadDist,
                  lateral, sd, aimTgtDist, srcDist, hp, maxHp, radius, subtendDeg,
                  onTarget ? 1 : 0, elevErr, elevBand, pitchMult);

    // ---- Full-autoaim makeability retarget --------------------------------
    // Hold the lock while the shot is makeable. Once it's been unmakeable past
    // the debounce (and no recent manual Q/E to honour), hand the lock to the
    // nearest fighter — range dominates crossingSpeed·range, so nearest is the
    // most makeable — instead of chasing this one through its unhittable orbit.
    // Silent (AnnounceSelectedTarget's locator ping only) so it never spams
    // speech. Assisted mode never auto-retargets (fullAuto gate).
    if (fullAuto && occCount > 0) {
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
}

}  // namespace acc::turret_game

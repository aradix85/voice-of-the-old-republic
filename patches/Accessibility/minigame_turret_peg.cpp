// DriveSelectedPeg: per tick, the audio peg that guides the aim onto the
// selected target (pan and pitch, the lock, the range and fire cues), with
// the lead and the aim assist from minigame_turret_assist.cpp.

#include "minigame_turret_internal.h"

#include "audio_bus.h"
#include "menus_modsettings.h"
#include "minigame_turret_lead.h"
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
// up/down axis was blind, the dominant miss at range (targets span -7°..+73°
// elevation). The orthogonal channel is PITCH: the
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

float Clamp1(float v) { return v > 1.0f ? 1.0f : (v < -1.0f ? -1.0f : v); }

// The census index of the locked fighter in `idx`, after keeping or moving
// the lock. False when there is nothing to drive this tick.
bool ResolveLock(int& idx, const int occSlot[], const float occDist[],
                 const Vector occPos[], const TargetInfo occInfo[], int occCount,
                 const Vector& listener, bool selectedAliveInPool) {
    if (idx >= 0) return true;

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
            return false;
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
            return false;
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
            return false;  // no fighters in range yet — wait
        }
    }
    return true;
}

// Range-window cue: punctuate the locked fighter crossing INTO killable
// range (enter) and back OUT of it (leave). Fires independent of the
// front-cone gate — it's about distance, not bearing. Same sample for both
// for now; the crosshair tone swelling (enter) vs fading (leave) is the
// current disambiguator. Played at a compressed, always-audible point in
// the fighter's direction so it pans toward it (the lock-locator trick).
void TickRangeCue(const Vector& sp, float sd, const TargetInfo& info,
                  const Vector& listener) {
    const Vector& ve = g_state.vel_ema;
    const float measured = g_state.have_vel
        ? std::sqrt(ve.x * ve.x + ve.y * ve.y + ve.z * ve.z) : 0.0f;
    const float killable = KillableRangeM(info.radius,
                                          TargetSpeed(&info, measured));
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

// "Fire now": one-shot the instant the combined aim crosses INTO the hitbox
// (false->true). Played at a compressed, always-audible point toward the
// fighter so it pans correctly. selected_on_target latches so it fires once
// per entry, not every tick you hold the cone.
void TickFireNowCue(bool onTargetCue, const Vector& sp, float sd,
                    const Vector& listener, float angle, float subtendDeg) {
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
}

// The peg itself: a solid loop while a shot connects, else ticks whose
// interval shrinks toward the cone. Returns the solid loop's source
// distance, for the log.
float DrivePegSound(bool onTargetCue, float angle, float onTargetAngle,
                    float subtendDeg, float aimTgtDist, const Vector& aimRel,
                    const Vector& tdir, const Vector& aimDir,
                    const Vector& listener) {
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
    return srcDist;
}

}  // namespace

float Wrap180(float d) {
    while (d > 180.0f)  d -= 360.0f;
    while (d < -180.0f) d += 360.0f;
    return d;
}

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

    if (!ResolveLock(idx, occSlot, occDist, occPos, occInfo, occCount, listener,
                     selectedAliveInPool)) {
        return;
    }

    if (!haveAim) {
        return;  // hold the cue; can't read the crosshair this tick
    }

    const Vector& sp = occPos[idx];
    const float   sd = occDist[idx];

    TickRangeCue(sp, sd, occInfo[idx], listener);

    // Fighter position relative to the listener (gun). We solve the whole lead
    // in this frame so the answer is "where to point the turret".
    const Vector P = {sp.x - listener.x, sp.y - listener.y, sp.z - listener.z};
    // |P|. Equal to the census distance sd when `listener` is the camera; a
    // hitscan area passes the barrel instead (see TickFighterCues).
    const float  pLen = std::sqrt(P.x * P.x + P.y * P.y + P.z * P.z);

    const ULONGLONG now = GetTickCount64();
    UpdateMotionEstimate(P, now);
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

    const Arena& arena = g_state.arena;
    const LeadSolution lead = SolveLead(
        P, (g_state.have_vel && !arena.hitscan) ? &v : nullptr,
        g_state.have_accel ? &g_state.accel_ema : nullptr,
        radius, arena.bolt_speed, arena.bolt_life);
    const Vector& aimRel    = lead.aimRel;
    const Vector& aimRelLin = lead.aimRelLin;
    const float   leadT     = lead.t;
    const float   leadDist  = lead.dist;
    const float   leadFrac  = lead.frac;

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
        (radius > 0.0f && aimTgtDist > 0.0f) ? std::atan(radius / aimTgtDist) * kRadToDeg : 0.0f;
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

    TickFireNowCue(onTargetCue, sp, sd, listener, angle, subtendDeg);
    const float srcDist = DrivePegSound(onTargetCue, angle, onTargetAngle, subtendDeg,
                                        aimTgtDist, aimRel, tdir, aimDir, listener);

    const float lateral = std::sin(angle * kDegToRad) * aimTgtDist;

    // Per-axis error to the INTERCEPT direction (the cue's target).
    const float aimAz = std::atan2(aimDir.x, aimDir.y) * kRadToDeg;
    const float aimEl = std::asin(Clamp1(aimDir.z))   * kRadToDeg;
    const float tgtAz = std::atan2(tdir.x,   tdir.y)  * kRadToDeg;
    const float tgtEl = std::asin(Clamp1(tdir.z))     * kRadToDeg;
    const float errAz = Wrap180(aimAz - tgtAz);
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
    if (haveOffset && haveAim) {
        CalibrateOffsetSigns(offAz, offEl, aimAz, aimEl);
    }

    const ULONGLONG nowMsA = GetTickCount64();
    float gain = 0.0f;
    const bool steer = SteerGun(haveOffset && haveAim, offAz, offEl, angle, errAz,
                                errEl, fullAuto, gain);

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
        const Vector wLin   = {listener.x + aimRelLin.x, listener.y + aimRelLin.y,
                               listener.z + aimRelLin.z};
        const Vector wCurve = {listener.x + aimRel.x, listener.y + aimRel.y,
                               listener.z + aimRel.z};
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

    if (fullAuto && occCount > 0) {
        AutoaimRetarget(justLandedHit, makeable, leadT, crossingSpeed, sd, occSlot,
                        occDist, occPos, occInfo, occCount, listener);
    }
}

}  // namespace acc::turret_game

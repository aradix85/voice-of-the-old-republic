# minigame_turret_peg.cpp (942 lines)

Source: `patches/Accessibility/minigame_turret_peg.cpp`

DriveSelectedPeg: per tick, the audio peg that guides the aim onto the selected target (pitch and pan, lock and fire cues, lead, aim assist).

## Declarations (in source order)

- L155 `float Wrap180(float d)`
- L161 `float Clamp1(float v)`
- L165 `bool ResolveLock(int& idx, const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener, bool selectedAliveInPool)`: The census index of the locked fighter in `idx`, after keeping or moving the lock.
- L240 `void TickRangeCue(const Vector& sp, float sd, const TargetInfo& info, const Vector& listener)`: Range-window cue: punctuate the locked fighter crossing INTO killable range (enter) and back OUT of it (leave).
- L265 `void UpdateMotionEstimate(const Vector& P, ULONGLONG now)`: ---- Smoothed relative velocity (Δ relative-pos / Δt, EMA).
- L330 `LeadSolution SolveLead(const Vector& P, const Vector& v, float vmag, float radius, const Arena& arena)`: ---- Lead: aim at the intercept point.
- L417 `void TickFireNowCue(bool onTargetCue, const Vector& sp, float sd, const Vector& listener, float angle, float subtendDeg)`: "Fire now": one-shot the instant the combined aim crosses INTO the hitbox (false->true).
- L437 `float DrivePegSound(bool onTargetCue, float angle, float onTargetAngle, float subtendDeg, float aimTgtDist, const Vector& aimRel, const Vector& tdir, const Vector& aimDir, const Vector& listener)`: The peg itself: a solid loop while a shot connects, else ticks whose interval shrinks toward the cone.
- L535 `void CalibrateOffsetSigns(float offAz, float offEl, float aimAz, float aimEl)`: Passive sign calibration: learn the sign of d(worldAim)/d(offset) per axis from frame-to-frame motion (player swing or our own writes).
- L565 `bool SteerGun(bool steer, float offAz, float offEl, float angle, float errAz, float errEl, bool fullAuto, float& gain)`: Steer the gun by writing offset toward the locked intercept.
- L634 `void AutoaimRetarget(bool justLandedHit, bool makeable, float leadT, float crossingSpeed, float sd, const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener)`: ---- Full-autoaim makeability retarget -------------------------------- Hold the lock while the shot is makeable.
- L698 `void DriveSelectedPeg(const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& aimDir, bool haveAim, const Vector& listener, bool selectedAliveInPool)`: Drive the continuous peg tone on the selected fighter.

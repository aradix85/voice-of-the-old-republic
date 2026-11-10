# minigame_turret_assist.cpp (296 lines)

Source: `patches/Accessibility/minigame_turret_assist.cpp`

The aim assist: the fighter's motion estimate, the predictive lead, the offset-sign calibration, steering the gun, and full-autoaim's retargeting.

## Declarations (in source order)

- L83 `void UpdateMotionEstimate(const Vector& P, ULONGLONG now)`: ---- Smoothed relative velocity (Δ relative-pos / Δt, EMA).
- L138 `void CalibrateOffsetSigns(float offAz, float offEl, float aimAz, float aimEl)`: Passive sign calibration: learn the sign of d(worldAim)/d(offset) per axis from frame-to-frame motion (player swing or our own writes).
- L168 `bool SteerGun(bool steer, float offAz, float offEl, float angle, float errAz, float errEl, bool fullAuto, float& gain)`: Steer the gun by writing offset toward the locked intercept.
- L237 `void AutoaimRetarget(bool justLandedHit, bool makeable, float leadT, float crossingSpeed, float sd, const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener)`: ---- Full-autoaim makeability retarget -------------------------------- Hold the lock while the shot is makeable.

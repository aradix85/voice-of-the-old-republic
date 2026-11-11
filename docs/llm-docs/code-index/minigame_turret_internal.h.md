# minigame_turret_internal.h (432 lines)

Source: `patches/Accessibility/minigame_turret_internal.h`

Internal to the minigame_turret module (minigame_turret*.cpp); no other file includes it. The public interface is minigame_turret.h. Here: what the files share.

## Declarations (in source order)

- L224 `struct Arena`: NOTE (engine-confirmed): the aim field (+0x1c4 offset) IS the aim — CSWMiniPlayer::Control integrates it and pushes it into the rotating gun/camera models each tick.
- L242 `struct TargetInfo`: Per-target values the cues need, read during the census.
- L249 `struct State`
- L398 `void ReadArena(void* miniGame)`: minigame_turret_arena.cpp
- L399 `float MakeHorizonSec(float radius, float speed)`
- L400 `float CensusRangeM()`
- L401 `float KillableRangeM(float radius, float speed)`
- L402 `float HumCompression()`
- L403 `float TargetSpeed(const TargetInfo* info, float measured)`
- L404 `bool ReadAimLine(Vector& outDir, Vector& outOrigin)`
- L405 `bool ReadOffset(float& azOut, float& elOut)`
- L406 `void WriteOffset(float az, float el)`
- L407 `float AngleBetweenDeg(const Vector& a, const Vector& b)`
- L410 `int CensusIndexOf(const int occSlot[], int occCount, int slot)`: minigame_turret_census.cpp
- L411 `int NumberForSlot(int slot)`
- L412 `void TickFighterCues()`
- L415 `void PushLeadProbe(int slot, const Vector& linW, const Vector& curveW, ULONGLONG dueMs)`: minigame_turret_targets.cpp
- L416 `void DrainLeadProbes(int slot, const Vector& fighterWorldNow, ULONGLONG now)`
- L417 `Vector PlayRangeCueAt(const Vector& targetPos, float dist, const Vector& listener)`
- L418 `void AnnounceSelectedTarget(int idx, const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener, bool speak = true)`
- L419 `void HandleTargetCycle(const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener)`
- L422 `void DriveSelectedPeg(const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& aimDir, bool haveAim, const Vector& listener, bool selectedAliveInPool)`: minigame_turret_peg.cpp
- L423 `float Wrap180(float d)`
- L426 `void UpdateMotionEstimate(const Vector& P, ULONGLONG now)`: minigame_turret_assist.cpp
- L427 `void CalibrateOffsetSigns(float offAz, float offEl, float aimAz, float aimEl)`
- L428 `bool SteerGun(bool steer, float offAz, float offEl, float angle, float errAz, float errEl, bool fullAuto, float& gain)`
- L429 `void AutoaimRetarget(bool justLandedHit, bool makeable, float leadT, float crossingSpeed, float sd, const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener)`

Also 37 constants or variables at namespace scope.

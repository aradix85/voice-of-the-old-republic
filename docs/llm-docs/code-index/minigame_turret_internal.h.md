# minigame_turret_internal.h (435 lines)

Source: `patches/Accessibility/minigame_turret_internal.h`

Internal to the minigame_turret module (minigame_turret*.cpp); no other file includes it. The public interface is minigame_turret.h. Here: what the files share.

## Declarations (in source order)

- L219 `struct Arena`: NOTE (engine-confirmed): the aim field (+0x1c4 offset) IS the aim — CSWMiniPlayer::Control integrates it and pushes it into the rotating gun/camera models each tick.
- L237 `struct TargetInfo`: Per-target values the cues need, read during the census.
- L244 `struct State`
- L392 `void ReadArena(void* miniGame)`: minigame_turret_arena.cpp
- L393 `float MakeHorizonSec(float radius, float speed)`
- L394 `float CensusRangeM()`
- L395 `float KillableRangeM(float radius, float speed)`
- L396 `float HumCompression()`
- L397 `float TargetSpeed(const TargetInfo* info, float measured)`
- L398 `bool ReadAimLine(Vector& outDir, Vector& outOrigin)`
- L399 `bool ReadOffset(float& azOut, float& elOut)`
- L400 `void WriteOffset(float az, float el)`
- L401 `float AngleBetweenDeg(const Vector& a, const Vector& b)`
- L404 `int CensusIndexOf(const int occSlot[], int occCount, int slot)`: minigame_turret_census.cpp
- L405 `int NumberForSlot(int slot)`
- L406 `void TickFighterCues()`
- L409 `void PushLeadProbe(int slot, const Vector& linW, const Vector& curveW, ULONGLONG dueMs)`: minigame_turret_targets.cpp
- L410 `void DrainLeadProbes(int slot, const Vector& fighterWorldNow, ULONGLONG now)`
- L411 `Vector PlayRangeCueAt(const Vector& targetPos, float dist, const Vector& listener)`
- L412 `void AnnounceSelectedTarget(int idx, const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener, bool speak = true)`
- L415 `void HandleTargetCycle(const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener)`
- L419 `void DriveSelectedPeg(const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& aimDir, bool haveAim, const Vector& listener, bool selectedAliveInPool)`: minigame_turret_peg.cpp
- L422 `float Wrap180(float d)`
- L425 `void UpdateMotionEstimate(const Vector& P, ULONGLONG now)`: minigame_turret_assist.cpp
- L426 `void CalibrateOffsetSigns(float offAz, float offEl, float aimAz, float aimEl)`
- L427 `bool SteerGun(bool steer, float offAz, float offEl, float angle, float errAz, float errEl, bool fullAuto, float& gain)`
- L429 `void AutoaimRetarget(bool justLandedHit, bool makeable, float leadT, float crossingSpeed, float sd, const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener)`

Also 39 constants or variables at namespace scope.

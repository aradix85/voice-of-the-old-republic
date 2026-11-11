# minigame_turret_internal.h (419 lines)

Source: `patches/Accessibility/minigame_turret_internal.h`

Internal to the minigame_turret module (minigame_turret*.cpp); no other file includes it. The public interface is minigame_turret.h. Here: what the files share.

## Declarations (in source order)

- L211 `struct Arena`: NOTE (engine-confirmed): the aim field (+0x1c4 offset) IS the aim — CSWMiniPlayer::Control integrates it and pushes it into the rotating gun/camera models each tick.
- L229 `struct TargetInfo`: Per-target values the cues need, read during the census.
- L236 `struct State`
- L385 `void ReadArena(void* miniGame)`: minigame_turret_arena.cpp
- L386 `float MakeHorizonSec(float radius, float speed)`
- L387 `float CensusRangeM()`
- L388 `float KillableRangeM(float radius, float speed)`
- L389 `float HumCompression()`
- L390 `float TargetSpeed(const TargetInfo* info, float measured)`
- L391 `bool ReadAimLine(Vector& outDir, Vector& outOrigin)`
- L392 `bool ReadOffset(float& azOut, float& elOut)`
- L393 `void WriteOffset(float az, float el)`
- L394 `float AngleBetweenDeg(const Vector& a, const Vector& b)`
- L397 `int CensusIndexOf(const int occSlot[], int occCount, int slot)`: minigame_turret_census.cpp
- L398 `int NumberForSlot(int slot)`
- L399 `void TickFighterCues()`
- L402 `void PushLeadProbe(int slot, const Vector& linW, const Vector& curveW, ULONGLONG dueMs)`: minigame_turret_targets.cpp
- L403 `void DrainLeadProbes(int slot, const Vector& fighterWorldNow, ULONGLONG now)`
- L404 `Vector PlayRangeCueAt(const Vector& targetPos, float dist, const Vector& listener)`
- L405 `void AnnounceSelectedTarget(int idx, const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener, bool speak = true)`
- L406 `void HandleTargetCycle(const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener)`
- L409 `void DriveSelectedPeg(const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& aimDir, bool haveAim, const Vector& listener, bool selectedAliveInPool)`: minigame_turret_peg.cpp
- L410 `float Wrap180(float d)`
- L413 `void UpdateMotionEstimate(const Vector& P, ULONGLONG now)`: minigame_turret_assist.cpp
- L414 `void CalibrateOffsetSigns(float offAz, float offEl, float aimAz, float aimEl)`
- L415 `bool SteerGun(bool steer, float offAz, float offEl, float angle, float errAz, float errEl, bool fullAuto, float& gain)`
- L416 `void AutoaimRetarget(bool justLandedHit, bool makeable, float leadT, float crossingSpeed, float sd, const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener)`

Also 36 constants or variables at namespace scope.

# minigame_turret_internal.h (418 lines)

Source: `patches/Accessibility/minigame_turret_internal.h`

Internal to the minigame_turret module (minigame_turret*.cpp); no other file includes it. The public interface is minigame_turret.h. Here: what the files share.

## Declarations (in source order)

- L210 `struct Arena`: NOTE (engine-confirmed): the aim field (+0x1c4 offset) IS the aim — CSWMiniPlayer::Control integrates it and pushes it into the rotating gun/camera models each tick.
- L228 `struct TargetInfo`: Per-target values the cues need, read during the census.
- L235 `struct State`
- L384 `void ReadArena(void* miniGame)`: minigame_turret_arena.cpp
- L385 `float MakeHorizonSec(float radius, float speed)`
- L386 `float CensusRangeM()`
- L387 `float KillableRangeM(float radius, float speed)`
- L388 `float HumCompression()`
- L389 `float TargetSpeed(const TargetInfo* info, float measured)`
- L390 `bool ReadAimLine(Vector& outDir, Vector& outOrigin)`
- L391 `bool ReadOffset(float& azOut, float& elOut)`
- L392 `void WriteOffset(float az, float el)`
- L393 `float AngleBetweenDeg(const Vector& a, const Vector& b)`
- L396 `int CensusIndexOf(const int occSlot[], int occCount, int slot)`: minigame_turret_census.cpp
- L397 `int NumberForSlot(int slot)`
- L398 `void TickFighterCues()`
- L401 `void PushLeadProbe(int slot, const Vector& linW, const Vector& curveW, ULONGLONG dueMs)`: minigame_turret_targets.cpp
- L402 `void DrainLeadProbes(int slot, const Vector& fighterWorldNow, ULONGLONG now)`
- L403 `Vector PlayRangeCueAt(const Vector& targetPos, float dist, const Vector& listener)`
- L404 `void AnnounceSelectedTarget(int idx, const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener, bool speak = true)`
- L405 `void HandleTargetCycle(const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener)`
- L408 `void DriveSelectedPeg(const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& aimDir, bool haveAim, const Vector& listener, bool selectedAliveInPool)`: minigame_turret_peg.cpp
- L409 `float Wrap180(float d)`
- L412 `void UpdateMotionEstimate(const Vector& P, ULONGLONG now)`: minigame_turret_assist.cpp
- L413 `void CalibrateOffsetSigns(float offAz, float offEl, float aimAz, float aimEl)`
- L414 `bool SteerGun(bool steer, float offAz, float offEl, float angle, float errAz, float errEl, bool fullAuto, float& gain)`
- L415 `void AutoaimRetarget(bool justLandedHit, bool makeable, float leadT, float crossingSpeed, float sd, const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener)`

Also 36 constants or variables at namespace scope.

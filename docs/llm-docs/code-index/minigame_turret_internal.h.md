# minigame_turret_internal.h (432 lines)

Source: `patches/Accessibility/minigame_turret_internal.h`

Internal to the minigame_turret module (minigame_turret*.cpp); no other file includes it. The public interface is minigame_turret.h. Here: what the files share.

## Declarations (in source order)

- L216 `struct Arena`: NOTE (engine-confirmed): the aim field (+0x1c4 offset) IS the aim — CSWMiniPlayer::Control integrates it and pushes it into the rotating gun/camera models each tick.
- L234 `struct TargetInfo`: Per-target values the cues need, read during the census.
- L241 `struct State`
- L389 `void ReadArena(void* miniGame)`: minigame_turret_arena.cpp
- L390 `float MakeHorizonSec(float radius, float speed)`
- L391 `float CensusRangeM()`
- L392 `float KillableRangeM(float radius, float speed)`
- L393 `float HumCompression()`
- L394 `float TargetSpeed(const TargetInfo* info, float measured)`
- L395 `bool ReadAimLine(Vector& outDir, Vector& outOrigin)`
- L396 `bool ReadOffset(float& azOut, float& elOut)`
- L397 `void WriteOffset(float az, float el)`
- L398 `float AngleBetweenDeg(const Vector& a, const Vector& b)`
- L401 `int CensusIndexOf(const int occSlot[], int occCount, int slot)`: minigame_turret_census.cpp
- L402 `int NumberForSlot(int slot)`
- L403 `void TickFighterCues()`
- L406 `void PushLeadProbe(int slot, const Vector& linW, const Vector& curveW, ULONGLONG dueMs)`: minigame_turret_targets.cpp
- L407 `void DrainLeadProbes(int slot, const Vector& fighterWorldNow, ULONGLONG now)`
- L408 `Vector PlayRangeCueAt(const Vector& targetPos, float dist, const Vector& listener)`
- L409 `void AnnounceSelectedTarget(int idx, const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener, bool speak = true)`
- L412 `void HandleTargetCycle(const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener)`
- L416 `void DriveSelectedPeg(const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& aimDir, bool haveAim, const Vector& listener, bool selectedAliveInPool)`: minigame_turret_peg.cpp
- L419 `float Wrap180(float d)`
- L422 `void UpdateMotionEstimate(const Vector& P, ULONGLONG now)`: minigame_turret_assist.cpp
- L423 `void CalibrateOffsetSigns(float offAz, float offEl, float aimAz, float aimEl)`
- L424 `bool SteerGun(bool steer, float offAz, float offEl, float angle, float errAz, float errEl, bool fullAuto, float& gain)`
- L426 `void AutoaimRetarget(bool justLandedHit, bool makeable, float leadT, float crossingSpeed, float sd, const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener)`

Also 39 constants or variables at namespace scope.

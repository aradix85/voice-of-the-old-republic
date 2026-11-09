# minigame_turret_internal.h (482 lines)

Source: `patches/Accessibility/minigame_turret_internal.h`

Internal to the minigame_turret module (minigame_turret*.cpp); no other file includes it. The public interface is minigame_turret.h. Here: what the files share.

## Declarations (in source order)

- L280 `struct Arena`: NOTE (engine-confirmed 2026-06-05): the aim field (+0x1c4 offset) IS the aim — CSWMiniPlayer::Control integrates it and pushes it into the rotating gun/camera models each tick.
- L298 `struct TargetInfo`: Per-target values the cues need, read during the census.
- L306 `struct State`
- L455 `void ReadArena(void* miniGame)`: minigame_turret_arena.cpp
- L456 `float MakeHorizonSec(float radius, float speed)`
- L457 `float CensusRangeM()`
- L458 `float KillableRangeM(float radius, float speed)`
- L459 `float HumCompression()`
- L460 `float TargetSpeed(const TargetInfo* info, float measured)`
- L461 `bool ReadAimLine(Vector& outDir, Vector& outOrigin)`
- L462 `bool ReadOffset(float& azOut, float& elOut)`
- L463 `void WriteOffset(float az, float el)`
- L464 `float AngleBetweenDeg(const Vector& a, const Vector& b)`
- L467 `int CensusIndexOf(const int occSlot[], int occCount, int slot)`: minigame_turret_census.cpp
- L468 `int NumberForSlot(int slot)`
- L469 `void TickFighterCues()`
- L472 `void PushLeadProbe(int slot, const Vector& linW, const Vector& curveW, ULONGLONG dueMs)`: minigame_turret_targets.cpp
- L473 `void DrainLeadProbes(int slot, const Vector& fighterWorldNow, ULONGLONG now)`
- L474 `Vector PlayRangeCueAt(const Vector& targetPos, float dist, const Vector& listener)`
- L475 `void AnnounceSelectedTarget(int idx, const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener, bool speak = true)`
- L476 `void HandleTargetCycle(const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener)`
- L479 `void DriveSelectedPeg(const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& aimDir, bool haveAim, const Vector& listener, bool selectedAliveInPool)`: minigame_turret_peg.cpp

Also 49 constants or variables at namespace scope.

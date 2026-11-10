# minigame_turret_internal.h (461 lines)

Source: `patches/Accessibility/minigame_turret_internal.h`

Internal to the minigame_turret module (minigame_turret*.cpp); no other file includes it. The public interface is minigame_turret.h. Here: what the files share.

## Declarations (in source order)

- L260 `struct Arena`: NOTE (engine-confirmed): the aim field (+0x1c4 offset) IS the aim — CSWMiniPlayer::Control integrates it and pushes it into the rotating gun/camera models each tick.
- L278 `struct TargetInfo`: Per-target values the cues need, read during the census.
- L285 `struct State`
- L434 `void ReadArena(void* miniGame)`: minigame_turret_arena.cpp
- L435 `float MakeHorizonSec(float radius, float speed)`
- L436 `float CensusRangeM()`
- L437 `float KillableRangeM(float radius, float speed)`
- L438 `float HumCompression()`
- L439 `float TargetSpeed(const TargetInfo* info, float measured)`
- L440 `bool ReadAimLine(Vector& outDir, Vector& outOrigin)`
- L441 `bool ReadOffset(float& azOut, float& elOut)`
- L442 `void WriteOffset(float az, float el)`
- L443 `float AngleBetweenDeg(const Vector& a, const Vector& b)`
- L446 `int CensusIndexOf(const int occSlot[], int occCount, int slot)`: minigame_turret_census.cpp
- L447 `int NumberForSlot(int slot)`
- L448 `void TickFighterCues()`
- L451 `void PushLeadProbe(int slot, const Vector& linW, const Vector& curveW, ULONGLONG dueMs)`: minigame_turret_targets.cpp
- L452 `void DrainLeadProbes(int slot, const Vector& fighterWorldNow, ULONGLONG now)`
- L453 `Vector PlayRangeCueAt(const Vector& targetPos, float dist, const Vector& listener)`
- L454 `void AnnounceSelectedTarget(int idx, const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener, bool speak = true)`
- L455 `void HandleTargetCycle(const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener)`
- L458 `void DriveSelectedPeg(const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& aimDir, bool haveAim, const Vector& listener, bool selectedAliveInPool)`: minigame_turret_peg.cpp

Also 49 constants or variables at namespace scope.

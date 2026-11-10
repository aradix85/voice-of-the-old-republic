# minigame_turret_peg.cpp (861 lines)

Source: `patches/Accessibility/minigame_turret_peg.cpp`

DriveSelectedPeg: per tick, the audio peg that guides the aim onto the selected target (pitch and pan, lock and fire cues, lead, aim assist).

## Declarations (in source order)

- L156 `void DriveSelectedPeg(const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& aimDir, bool haveAim, const Vector& listener, bool selectedAliveInPool)`: Drive the continuous peg tone on the selected fighter.

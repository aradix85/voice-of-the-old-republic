# minigame_turret_peg.cpp (531 lines)

Source: `patches/Accessibility/minigame_turret_peg.cpp`

DriveSelectedPeg: per tick, the audio peg that guides the aim onto the selected target (pitch and pan, lock and fire cues, lead, aim assist).

## Declarations (in source order)

- L52 `float Clamp1(float v)`
- L56 `bool ResolveLock(int& idx, const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener, bool selectedAliveInPool)`: The census index of the locked fighter in `idx`, after keeping or moving the lock.
- L131 `void TickRangeCue(const Vector& sp, float sd, const TargetInfo& info, const Vector& listener)`: Range-window cue: punctuate the locked fighter crossing INTO killable range (enter) and back OUT of it (leave).
- L159 `void TickFireNowCue(bool onTargetCue, const Vector& sp, float sd, const Vector& listener, float angle, float subtendDeg)`: "Fire now": one-shot the instant the combined aim crosses INTO the hitbox (false->true).
- L179 `float DrivePegSound(bool onTargetCue, float angle, float onTargetAngle, float subtendDeg, float aimTgtDist, const Vector& aimRel, const Vector& tdir, const Vector& aimDir, const Vector& listener)`: The peg itself: a solid loop while a shot connects, else ticks whose interval shrinks toward the cone.
- L276 `float Wrap180(float d)`
- L287 `void DriveSelectedPeg(const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& aimDir, bool haveAim, const Vector& listener, bool selectedAliveInPool)`: Drive the continuous peg tone on the selected fighter.

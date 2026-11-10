# minigame_swoop_audio_bars.cpp (248 lines)

Source: `patches/Accessibility/minigame_swoop_audio_bars.cpp`

K2 bars and mines of the swoop race: the free gap beside a half bar, the jump-now cue (and its parked variant), and the mine-pass log.

## Declarations (in source order)

- L20 `float MineLateralReach(int m, float bikeZ, float /*playerRadius*/)`: Lateral reach of mine `m` at the bike's ground height: sqrt(R² − dz²) plus slack, R = the measured combined reach (kMineCollisionRadiusU — the live sphere_radius is NOT the collision radius, see there).
- L29 `float WidestStripPiece(float lo, float hi, float barY, float bikeZ, float playerRadius, float& outLo, float& outHi)`: One side's strip [lo, hi] minus every mine near the bar.
- L59 `void ComputeBarGap(SpatialAudioState::BarInfo& bar, float bikeZ, float playerRadius)`: Fills bar.has_gap / gap_x / gap_side (-1 left strip, +1 right strip).
- L82 `void TrackMinePasses(void* player, const Vector& bikePos, float playerRadius)`: Mine-pass log: per mine, the closest 3-D approach while the mine is within kMinePassWindowU ahead or behind, logged once when it falls behind, with the hp before and after.
- L123 `void HandleParkedBar(int i, const ObstacleView& v, HazardInfo& hz, const SpatialAudioState::BarInfo& cur, float gap, float bikeX)`: Parked right in front of this bar, inside its span: classify the escape (see ParkedBar in the header) for the stuck hint, and, off the wall with no strip, play the centred beep once per park: "jump first, then W".
- L152 `float JumpLead(int slot, float y0, float vWorld, bool& pair)`: The jump lead: the single-bar constant, or, when the next bar behind this one is close enough that one jump clears both at this speed with a press window of at least kPairMinWindowSec, the later "pair" lead that centr...
- L176 `void MaybeJumpCue(int i, const ObstacleView& v, HazardInfo& hz, const SpatialAudioState::BarInfo& cur, float gap, float bikeX, const ObstacleTick& t)`: First beep: "press now".
- L215 `void HandleBar(int i, const ObstacleView& v, HazardInfo& hz, ObstacleTick& t)`: A low bar (K2): the jump-now cue, never the warn loop.

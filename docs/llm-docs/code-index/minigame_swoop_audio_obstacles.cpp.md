# minigame_swoop_audio_obstacles.cpp (651 lines)

Source: `patches/Accessibility/minigame_swoop_audio_obstacles.cpp`

Obstacle cues of the swoop race: which objects are hazards (scenery, mines, bars, posts), their boxes and the gap in a bar, mines passed, and the per-tick obstacle cues.

## Declarations (in source order)

- L83 `int ObstacleLoopBudget()`: Cap on how many obstacle loops may sound at once, nearest first.
- L90 `bool IsSceneryObstacleName(const char* name)`: The ` _MGE*` scenery objects (see the constants block): the token "_MGE" anywhere in the resref, case-insensitive.
- L104 `void ResolveHazardBox(void* aur, const char* name, HazardInfo& hz)`: Resolve and classify one obstacle's collision box (KOTOR 2).
- L133 `float MineLateralReach(int m, float bikeZ, float /*playerRadius*/)`: Lateral reach of mine `m` at the bike's ground height: sqrt(R² − dz²) plus slack, R = the measured combined reach (kMineCollisionRadiusU — the live sphere_radius is NOT the collision radius, see there).
- L142 `float WidestStripPiece(float lo, float hi, float barY, float bikeZ, float playerRadius, float& outLo, float& outHi)`: One side's strip [lo, hi] minus every mine near the bar.
- L172 `void ComputeBarGap(SpatialAudioState::BarInfo& bar, float bikeZ, float playerRadius)`: Fills bar.has_gap / gap_x / gap_side (-1 left strip, +1 right strip).
- L195 `void TrackMinePasses(void* player, const Vector& bikePos, float playerRadius)`: Mine-pass log: per mine, the closest 3-D approach while the mine is within kMinePassWindowU ahead or behind, logged once when it falls behind, with the hp before and after.
- L268 `bool ReadObstacleTick(void* miniGame, ObstacleTick& t)`: Bike state for the K2 bar logic (lane, forward position, speed, the airborne edge for the "jump again" repeat) and the listener; false without a listener anchor.
- L312 `bool ReadObstacle(void* mgoArray, int i, ObstacleTick& t, ObstacleView& v)`
- L352 `void LogObstacleInventory(int i, const ObstacleView& v, const HazardInfo& hz, bool k2)`: First sweep only: every obstacle's slot and name (and the K2 box and class), so the per-track inventory is in the log once.
- L380 `void HandleParkedBar(int i, const ObstacleView& v, HazardInfo& hz, const SpatialAudioState::BarInfo& cur, float gap, float bikeX)`: Parked right in front of this bar, inside its span: classify the escape (see ParkedBar in the header) for the stuck hint, and, off the wall with no strip, play the centred beep once per park: "jump first, then W".
- L409 `float JumpLead(int slot, float y0, float vWorld, bool& pair)`: The jump lead: the single-bar constant, or, when the next bar behind this one is close enough that one jump clears both at this speed with a press window of at least kPairMinWindowSec, the later "pair" lead that centr...
- L433 `void MaybeJumpCue(int i, const ObstacleView& v, HazardInfo& hz, const SpatialAudioState::BarInfo& cur, float gap, float bikeX, const ObstacleTick& t)`: First beep: "press now".
- L472 `void HandleBar(int i, const ObstacleView& v, HazardInfo& hz, ObstacleTick& t)`: A low bar (K2): the jump-now cue, never the warn loop.
- L508 `bool Admit(ObstacleTick& t, int slot, const Vector& hazardPos, const char* name)`: A hazard that warns through the loop (K1 rocks, K2 blocks and mines): ahead of the listener within the margin, and inside range.
- L524 `int SelectNearest(ObstacleTick& t)`: Nearest first, cut to the loop budget: a partial selection sort (single digits on K2; on K1 the budget is the whole array).
- L544 `int SoundWarnLoops(const ObstacleTick& t, int sounding, bool active[kMgoArraySlotCount])`: The decoupled cue: lateral and vertical true, forward pinned to a fixed reference depth, so the pan says lane error only; loudness follows our own curve on the real 3D distance.
- L580 `int StopStaleLoops(const bool active[kMgoArraySlotCount])`: Loops whose obstacle left range (passed, or its reads failed this tick); Stop is idempotent.
- L593 `void TickObstacleCues(void* miniGame)`

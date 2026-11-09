# minigame_swoop_audio_obstacles.cpp (637 lines)

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
- L231 `void TickObstacleCues(void* miniGame)`

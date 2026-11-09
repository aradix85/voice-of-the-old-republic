# minigame_swoop_audio.cpp (2444 lines)

Source: `patches/Accessibility/minigame_swoop_audio.cpp`

Swoop race spatial-audio implementation — see swoop_spatial_audio.h.

Split from swoop_race.cpp on 2026-05-27 (large-file-handling pass). The race lifecycle (entry/exit, gear watch, wall collision, acceleration speedometer) stays in swoop_race.cpp; everything that walks the 255-slot CSWMiniGameObjectArray to surface obstacles and accelpads as 3D loops lives here.

## Declarations (in source order)

- L596 `inline float WallLeadTicks()`
- L599 `inline float WallEdgeOffsetU()`
- L769 `inline float AssistGapU()`
- L777 `inline float ReleaseBand()`: Post-coast miss within the assist gap reads as "on the line" → release/hold: the magnet finishes from here, so the player need not nail dead-center by ear.
- L834 `inline float MagnetEngageU()`: ----- Lateral steering magnetism (2026-06-22, shared aim-assist port) ------- The console-style "sticky aim" from the shipped turret assist, applied to the swoop bike's ONE steering axis.
- L891 `inline int SwoopVolumeByte(float dist, float range)`: Linear volume byte for a source at 3D distance `dist` within `range`: full (kSwoopVolNearByte) at the listener, ramping to kSwoopVolFarByte at the range edge.
- L1044 `const char* ReadAurObjectName(void* aurObject)`: Returns the CAurObject's name pointer (engine-owned, valid for the lifetime of the object), or nullptr on any null link / SEH fault.
- L1102 `bool TrackAxisUsable(void* miniGame)`
- L1151 `int ObstacleLoopBudget()`: Cap on how many obstacle loops may sound at once, nearest first.
- L1158 `bool IsSceneryObstacleName(const char* name)`: The ` _MGE*` scenery objects (see the constants block): the token "_MGE" anywhere in the resref, case-insensitive.
- L1172 `void ResolveHazardBox(void* aur, const char* name, HazardInfo& hz)`: Resolve and classify one obstacle's collision box (KOTOR 2).
- L1201 `float MineLateralReach(int m, float bikeZ, float /*playerRadius*/)`: Lateral reach of mine `m` at the bike's ground height: sqrt(R² − dz²) plus slack, R = the measured combined reach (kMineCollisionRadiusU — the live sphere_radius is NOT the collision radius, see there).
- L1210 `float WidestStripPiece(float lo, float hi, float barY, float bikeZ, float playerRadius, float& outLo, float& outHi)`: One side's strip [lo, hi] minus every mine near the bar.
- L1240 `void ComputeBarGap(SpatialAudioState::BarInfo& bar, float bikeZ, float playerRadius)`: Fills bar.has_gap / gap_x / gap_side (-1 left strip, +1 right strip).
- L1261 `void TrackMinePasses(void* player, const Vector& bikePos, float playerRadius)`: Mine-pass log: per mine, the closest 3-D approach while the mine is within kMinePassWindowU ahead or behind, logged once when it falls behind, with the hp before and after.
- L1297 `void TickObstacleCues(void* miniGame)`
- L1744 `void TickAccelpadCues(void* miniGame)`
- L2388 `void TickSpatialAudio(void* miniGame)`
- L2396 `ParkedBar GetParkedBarSituation()`
- L2400 `void ResetSpatialAudio()`

# minigame_swoop_audio_obstacles.cpp (341 lines)

Source: `patches/Accessibility/minigame_swoop_audio_obstacles.cpp`

Obstacle cues of the swoop race: which objects are hazards (scenery, mines, bars, posts), their boxes and the gap in a bar, mines passed, and the per-tick obstacle cues.

## Declarations (in source order)

- L53 `int ObstacleLoopBudget()`: Cap on how many obstacle loops may sound at once, nearest first.
- L60 `bool IsSceneryObstacleName(const char* name)`: The ` _MGE*` scenery objects (see the constants block): the token "_MGE" anywhere in the resref, case-insensitive.
- L74 `void ResolveHazardBox(void* aur, const char* name, HazardInfo& hz)`: Resolve and classify one obstacle's collision box (KOTOR 2).
- L103 `bool ReadObstacleTick(void* miniGame, ObstacleTick& t)`: Bike state for the K2 bar logic (lane, forward position, speed, the airborne edge for the "jump again" repeat) and the listener; false without a listener anchor.
- L131 `bool ReadObstacle(void* mgoArray, int i, ObstacleTick& t, ObstacleView& v)`
- L171 `void LogObstacleInventory(int i, const ObstacleView& v, const HazardInfo& hz, bool k2)`: First sweep only: every obstacle's slot and name (and the K2 box and class), so the per-track inventory is in the log once.
- L198 `bool Admit(ObstacleTick& t, int slot, const Vector& hazardPos, const char* name)`: A hazard that warns through the loop (K1 rocks, K2 blocks and mines): ahead of the listener within the margin, and inside range.
- L214 `int SelectNearest(ObstacleTick& t)`: Nearest first, cut to the loop budget: a partial selection sort (single digits on K2; on K1 the budget is the whole array).
- L234 `int SoundWarnLoops(const ObstacleTick& t, int sounding, bool active[kMgoArraySlotCount])`: The decoupled cue: lateral and vertical true, forward pinned to a fixed reference depth, so the pan says lane error only; loudness follows our own curve on the real 3D distance.
- L270 `int StopStaleLoops(const bool active[kMgoArraySlotCount])`: Loops whose obstacle left range (passed, or its reads failed this tick); Stop is idempotent.
- L283 `void TickObstacleCues(void* miniGame)`

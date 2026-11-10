# minigame_swoop_audio_obstacles.cpp (318 lines)

Source: `patches/Accessibility/minigame_swoop_audio_obstacles.cpp`

Obstacle cues of the swoop race: each tick sweeps the minigame's objects, classifies the hazards by their collision box (K2: scenery, posts, bars, blocks), hands the bars to minigame_swoop_audio_bars.cpp, and sounds a panned warn loop for the nearest blocks, rocks and mines.

## Declarations (in source order)

- L34 `int ObstacleLoopBudget()`: How many warn loops may sound at once, nearest first.
- L41 `bool IsSceneryObstacleName(const char* name)`: The ` _MGE*` scenery objects (see the constants block): the token "_MGE" anywhere in the resref, case-insensitive.
- L55 `void ResolveHazardBox(void* aur, const char* name, HazardInfo& hz)`: Resolve and classify one obstacle's collision box (KOTOR 2).
- L80 `bool ReadObstacleTick(void* miniGame, ObstacleTick& t)`: Bike state for the K2 bar logic (lane, forward position, speed, the airborne edge for the "jump again" repeat) and the listener; false without a listener anchor.
- L108 `bool ReadObstacle(void* mgoArray, int i, ObstacleTick& t, ObstacleView& v)`
- L148 `void LogObstacleInventory(int i, const ObstacleView& v, const HazardInfo& hz, bool k2)`: First sweep only: every obstacle's slot and name (and the K2 box and class), so the per-track inventory is in the log once.
- L175 `bool Admit(ObstacleTick& t, int slot, const Vector& hazardPos, const char* name)`: A hazard that warns through the loop (K1 rocks, K2 blocks and mines): ahead of the listener within the margin, and inside range.
- L191 `int SelectNearest(ObstacleTick& t)`: Nearest first, cut to the loop budget: a partial selection sort (single digits on K2; on K1 the budget is the whole array).
- L211 `int SoundWarnLoops(const ObstacleTick& t, int sounding, bool active[kMgoArraySlotCount])`: The decoupled cue: lateral and vertical true, forward pinned to a fixed reference depth, so the pan says lane error only; loudness follows our own curve on the real 3D distance.
- L247 `int StopStaleLoops(const bool active[kMgoArraySlotCount])`: Loops whose obstacle left range (passed, or its reads failed this tick); Stop is idempotent.
- L260 `void TickObstacleCues(void* miniGame)`

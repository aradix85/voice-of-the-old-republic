# minigame_engine.cpp (117 lines)

Source: `patches/Accessibility/minigame_engine.cpp`

(From minigame_engine.h.) What the minigame files share on the engine side: the area's minigame, the minigame object array, a track follower's rendered position and bounding boxes, and the offset field that steers the controlled object.

Both the swoop bike and the turret gun are a CSWMiniPlayer (extends CSWTrackFollower). Each holds an `offset` Vector that CSWMiniPlayer::Control integrates every tick (`offset += axis-velocity`) and then applies to the controlled object: - turret: offset.x = elevation, offset.z = azimuth, in degrees; - swoop:  offset.x = the lateral lane coordinate (world-X tracks tunnel-X 1:1, so a unit of offset.x is a unit of world-X). Because the engine re-integrates offset from its current value each tick, a write issued after Control has run (from the OnUpdate tick, where the minigame modules live) sticks: the next tick only adds the player's own small delta on top. That is how the aim assists steer.

## Declarations (in source order)

- L13 `void* ReadAreaMiniGame()`
- L19 `LatchStep TickLatch(Latch& latch, uint32_t type, const char* logTag)`
- L35 `void* ResolveMgoArray()`
- L39 `void* ReadFollowerModelGob(void* follower)`
- L45 `bool ReadFollowerPosition(void* follower, Vector& out)`
- L61 `bool ReadGobBoundingBox(void* gob, Vector& mn, Vector& mx)`
- L77 `bool ReadGobPartBoundingBox(void* gob, const char* partName, Vector& mn, Vector& mx)`
- L103 `bool ReadOffsetVector(void* player, Vector& out)`
- L107 `void WriteOffsetVector(void* player, const Vector& v)`

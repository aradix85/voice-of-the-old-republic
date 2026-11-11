# minigame_engine.cpp (113 lines)

Source: `patches/Accessibility/minigame_engine.cpp`

(From minigame_engine.h.) What the minigame files share on the engine side: the area's minigame, the minigame object array, a track follower's rendered position and bounding boxes, and the offset field that steers the controlled object.

Both the swoop bike and the turret gun are a CSWMiniPlayer (extends CSWTrackFollower). Each holds an `offset` Vector that CSWMiniPlayer::Control integrates every tick (`offset += axis-velocity`) and then applies to the controlled object: - turret: offset.x = elevation, offset.z = azimuth, in degrees; - swoop:  offset.x = the lateral lane coordinate (world-X tracks tunnel-X 1:1, so a unit of offset.x is a unit of world-X). Because the engine re-integrates offset from its current value each tick, a write issued after Control has run (from the OnUpdate tick, where the minigame modules live) sticks: the next tick only adds the player's own small delta on top. That is how the aim assists steer.

## Declarations (in source order)

- L14 `void* ReadAreaMiniGame()`
- L20 `LatchStep TickLatch(Latch& latch, uint32_t type, const char* logTag)`
- L36 `void* ResolveMgoArray()`
- L40 `void* ReadFollowerModelGob(void* follower)`
- L46 `bool ReadFollowerPosition(void* follower, Vector& out)`
- L62 `bool ReadGobBoundingBox(void* gob, Vector& mn, Vector& mx)`
- L78 `bool ReadGobPartBoundingBox(void* gob, const char* partName, Vector& mn, Vector& mx)`
- L104 `bool ReadOffsetVector(void* player, Vector& out)`
- L108 `void WriteOffsetVector(void* player, const Vector& v)`

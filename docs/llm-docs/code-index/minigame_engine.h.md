# minigame_engine.h (119 lines)

Source: `patches/Accessibility/minigame_engine.h`

What the minigame files share on the engine side: the area's minigame, the minigame object array, a track follower's rendered position and bounding boxes, and the offset field that steers the controlled object.

Both the swoop bike and the turret gun are a CSWMiniPlayer (extends CSWTrackFollower). Each holds an `offset` Vector that CSWMiniPlayer::Control integrates every tick (`offset += axis-velocity`) and then applies to the controlled object: - turret: offset.x = elevation, offset.z = azimuth, in degrees; - swoop:  offset.x = the lateral lane coordinate (world-X tracks tunnel-X 1:1, so a unit of offset.x is a unit of world-X). Because the engine re-integrates offset from its current value each tick, a write issued after Control has run (from the OnUpdate tick, where the minigame modules live) sticks: the next tick only adds the player's own small delta on top. That is how the aim assists steer.

## Declarations (in source order)

- L38 `void* ReadAreaMiniGame()`: CSWCArea.mini_game of the player's area, or nullptr.
- L51 `void* ResolveMgoArray()`: Returns the minigame object array, or nullptr if any link of the chain is null or faults (i.e.
- L55 `void* CallAsCast(void* obj, size_t vtableSlotOffset)`: Call a vtable AsSWCxxx-style downcast at `vtableSlotOffset` on `obj`.
- L77 `bool ReadFollowerPosition(void* follower, Vector& out)`: World position of `follower`'s first model.
- L82 `void* ReadFollowerModelGob(void* follower)`: `follower`'s first model object — a Gob (render object), the same class the obstacles' CAurObject slot points at, so the Gob accessors below apply to it (GetModelName tells a K2 accelerator pad from a mine).
- L106 `bool ReadGobBoundingBox(void* gob, Vector& mn, Vector& mx)`: Model-local bounding box of a Gob (whole model).
- L109 `bool ReadGobPartBoundingBox(void* gob, const char* partName, Vector& mn, Vector& mx)`: Model-local bounding box of one named part of a Gob.
- L115 `bool ReadOffsetVector(void* player, Vector& out)`: Read / write the whole offset Vector on a CSWMiniPlayer `player` pointer.
- L116 `void WriteOffsetVector(void* player, const Vector& v)`

Also 10 constants or variables at namespace scope.

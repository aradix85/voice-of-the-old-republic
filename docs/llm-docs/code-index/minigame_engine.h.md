# minigame_engine.h (65 lines)

Source: `patches/Accessibility/minigame_engine.h`

What the minigame files share on the engine side: the area's minigame, the minigame object array, a track follower's rendered position and bounding boxes, and the offset field that steers the controlled object.

Both the swoop bike and the turret gun are a CSWMiniPlayer (extends CSWTrackFollower). Each holds an `offset` Vector that CSWMiniPlayer::Control integrates every tick (`offset += axis-velocity`) and then applies to the controlled object: - turret: offset.x = elevation, offset.z = azimuth, in degrees; - swoop:  offset.x = the lateral lane coordinate (world-X tracks tunnel-X 1:1, so a unit of offset.x is a unit of world-X). Because the engine re-integrates offset from its current value each tick, a write issued after Control has run (from the OnUpdate tick, where the minigame modules live) sticks: the next tick only adds the player's own small delta on top. That is how the aim assists steer.

## Declarations (in source order)

- L34 `void* ReadAreaMiniGame()`: CSWCArea.mini_game of the player's area, or nullptr.
- L38 `void* ResolveMgoArray()`: The minigame object array (obstacles, accelerator pads, track followers), or nullptr when no minigame is live.
- L44 `bool ReadFollowerPosition(void* follower, Vector& out)`: World position of `follower`'s first model: the model is what the engine renders and what the spatial cues must track, and the follower's own transform is not the same.
- L50 `void* ReadFollowerModelGob(void* follower)`: `follower`'s first model object: a Gob (render object), the class the obstacles' CAurObject slot points at, so the two readers below apply to it (and GetModelName, which tells a KOTOR 2 accelerator pad from a mine).
- L53 `bool ReadGobBoundingBox(void* gob, Vector& mn, Vector& mx)`: Model-local bounding box of a Gob (whole model).
- L56 `bool ReadGobPartBoundingBox(void* gob, const char* partName, Vector& mn, Vector& mx)`: Model-local bounding box of one named part of a Gob.
- L61 `bool ReadOffsetVector(void* player, Vector& out)`: The whole offset Vector of a CSWMiniPlayer.
- L62 `void WriteOffsetVector(void* player, const Vector& v)`

Also 4 constants or variables at namespace scope.

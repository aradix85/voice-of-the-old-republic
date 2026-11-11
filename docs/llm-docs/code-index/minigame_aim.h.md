# minigame_aim.h (163 lines)

Source: `patches/Accessibility/minigame_aim.h`

Shared minigame aim-assist facility — the engine-grounded primitives that let us STEER a minigame's controlled object by writing its integration field, plus the console-style "sticky magnetism" math.

Both the swoop bike and the turret gun are a CSWMiniPlayer (extends CSWTrackFollower). Each holds an `offset` Vector at +0x1c4 that CSWMiniPlayer::Control integrates every tick (`offset += axis-velocity`) and then applies to the controlled object: - turret: offset.x = elevation°, offset.z = azimuth° (the gun/camera aim); - swoop:  offset.x = lateral lane coordinate (the bike's position in the tunnel; world-X tracks tunnel-X 1:1, so a unit of offset.x is a unit of world-X). Because the engine RE-INTEGRATES offset from its current value each tick, a per-tick WRITE (issued after Control has run — i.e. from the OnUpdate tick where the minigame modules live) sticks: the next tick only adds the player's own small delta on top. This is the mechanism the shipped turret aim-assist uses; this header lifts the reusable parts out so swoop can share them.

What is shared (here): the offset read/write primitives and the magnetism curve + per-tick-capped step, plus the SEH read primitives and minigame object-array resolution all three minigame TUs need. What stays per-game (in minigame_turret.cpp / minigame_swoop_audio.cpp): how the aim ERROR is computed (angular for the turret, linear lane-units for swoop), target selection, and the offset→world SIGN (the turret calibrates it; swoop's 1:1 mapping passes +1). All reads SEH-guarded.

## Declarations (in source order)

- L56 `void* ResolveMgoArray()`: Returns the minigame object array, or nullptr if any link of the chain is null or faults (i.e.
- L60 `void* CallAsCast(void* obj, size_t vtableSlotOffset)`: Call a vtable AsSWCxxx-style downcast at `vtableSlotOffset` on `obj`.
- L82 `bool ReadFollowerPosition(void* follower, Vector& out)`: World position of `follower`'s first model.
- L87 `void* ReadFollowerModelGob(void* follower)`: `follower`'s first model object — a Gob (render object), the same class the obstacles' CAurObject slot points at, so the Gob accessors below apply to it (GetModelName tells a K2 accelerator pad from a mine).
- L111 `bool ReadGobBoundingBox(void* gob, Vector& mn, Vector& mx)`: Model-local bounding box of a Gob (whole model).
- L114 `bool ReadGobPartBoundingBox(void* gob, const char* partName, Vector& mn, Vector& mx)`: Model-local bounding box of one named part of a Gob.
- L135 `bool ReadOffsetVector(void* player, Vector& out)`: Read / write the whole offset Vector on a CSWMiniPlayer `player` pointer.
- L136 `void WriteOffsetVector(void* player, const Vector& v)`
- L144 `struct MagnetParams`: ---- Console-style sticky magnetism (the "less invasive" assist) ----------- A proximity-ramped pull on one offset axis: gentle far from the target (a guide you can steer straight through to pick a different target) a...
- L152 `float MagnetGain(float t, const MagnetParams& p)`: Proximity-ramped gain.
- L159 `float MagnetStep(float offsetVal, float mappedErr, float gain, const MagnetParams& p)`: One tick of magnetism on a single axis.

Also 11 constants or variables at namespace scope.

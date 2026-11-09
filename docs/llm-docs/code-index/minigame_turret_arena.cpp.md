# minigame_turret_arena.cpp (269 lines)

Source: `patches/Accessibility/minigame_turret_arena.cpp`

The turret's world: the gun model and its aim line, the arena (tunnel bounds, bolt speed and life), the ranges derived from it (make, census, killable), target speed, the player's aim offset and angles.

## Declarations (in source order)

- L76 `Vector QuatRotate(const Quat& q, const Vector& v)`: Rotate a vector by a quaternion (w,x,y,z): v' = v + 2w(u×v) + 2(u×(u×v)), u = (x,y,z).
- L90 `void* ResolveGunBank0(void* miniGame)`: Resolve the player's first gun bank (CSWMGGunBank*).
- L100 `void* ResolveGunModel()`: Resolve the player's first gun model (CAurObject*) for the bullethook read.
- L114 `void ReadArena(void* miniGame)`: Read the area's ballistics and aim limits into g_state.arena.
- L166 `float MakeHorizonSec(float radius, float speed)`: How far ahead (seconds of bolt flight) the lead can predict a target of this radius and speed before its miss outgrows the hitbox.
- L178 `float CensusRangeM()`: Census / Q/E range.
- L187 `float KillableRangeM(float radius, float speed)`: Distance inside which the range cue says "you can hit this".
- L195 `float HumCompression()`
- L202 `float TargetSpeed(const TargetInfo* info, float measured)`: The speed the range model should use for a target: the engine's track speed for a fighter, else the measured one.
- L214 `bool ReadAimLine(Vector& outDir, Vector& outOrigin)`: THE CROSSHAIR.
- L242 `bool ReadOffset(float& azOut, float& elOut)`: Read / write the aim field (CSWMiniPlayer.offset +0x1c4): x = elevation°, z = azimuth°.
- L251 `void WriteOffset(float az, float el)`
- L262 `float AngleBetweenDeg(const Vector& a, const Vector& b)`: Angle (degrees) between the aim ray and the (already normalised) direction to a target.

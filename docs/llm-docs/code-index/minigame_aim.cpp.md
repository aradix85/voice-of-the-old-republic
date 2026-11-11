# minigame_aim.cpp (170 lines)

Source: `patches/Accessibility/minigame_aim.cpp`

Shared minigame aim-assist facility — see minigame_aim.h for the design and the engine grounding (CSWMiniPlayer.offset +0x1c4 is the per-tick-integrated aim/lane field; writing it after Control runs steers the controlled object).

## Declarations (in source order)

- L14 `void* ReadAreaMiniGame()`
- L22 `void* ResolveMgoArray()`
- L28 `bool ReadFollowerPosition(void* follower, Vector& out)`
- L57 `void* ReadFollowerModelGob(void* follower)`
- L68 `bool ReadGobBoundingBox(void* gob, Vector& mn, Vector& mx)`
- L87 `bool ReadGobPartBoundingBox(void* gob, const char* partName, Vector& mn, Vector& mx)`
- L118 `void* CallAsCast(void* obj, size_t vtableSlotOffset)`
- L133 `bool ReadOffsetVector(void* player, Vector& out)`
- L145 `void WriteOffsetVector(void* player, const Vector& v)`
- L155 `float MagnetGain(float t, const MagnetParams& p)`
- L161 `float MagnetStep(float offsetVal, float mappedErr, float gain, const MagnetParams& p)`

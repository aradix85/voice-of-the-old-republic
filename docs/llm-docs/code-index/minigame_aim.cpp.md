# minigame_aim.cpp (163 lines)

Source: `patches/Accessibility/minigame_aim.cpp`

Shared minigame aim-assist facility — see minigame_aim.h for the design and the engine grounding (CSWMiniPlayer.offset +0x1c4 is the per-tick-integrated aim/lane field; writing it after Control runs steers the controlled object).

## Declarations (in source order)

- L15 `void* ResolveMgoArray()`
- L21 `bool ReadFollowerPosition(void* follower, Vector& out)`
- L50 `void* ReadFollowerModelGob(void* follower)`
- L61 `bool ReadGobBoundingBox(void* gob, Vector& mn, Vector& mx)`
- L80 `bool ReadGobPartBoundingBox(void* gob, const char* partName, Vector& mn, Vector& mx)`
- L111 `void* CallAsCast(void* obj, size_t vtableSlotOffset)`
- L126 `bool ReadOffsetVector(void* player, Vector& out)`
- L138 `void WriteOffsetVector(void* player, const Vector& v)`
- L148 `float MagnetGain(float t, const MagnetParams& p)`
- L154 `float MagnetStep(float offsetVal, float mappedErr, float gain, const MagnetParams& p)`

# minigame_aim.cpp (209 lines)

Source: `patches/Accessibility/minigame_aim.cpp`

Shared minigame aim-assist facility — see minigame_aim.h for the design and the engine grounding (CSWMiniPlayer.offset +0x1c4 is the per-tick-integrated aim/lane field; writing it after Control runs steers the controlled object).

## Declarations (in source order)

- L18 `void* SafeReadPtr(void* base, size_t off)`
- L28 `uint32_t SafeReadU32(void* base, size_t off)`
- L38 `float SafeReadFloat(void* base, size_t off)`
- L48 `bool SafeReadVector(void* base, size_t off, Vector& out)`
- L61 `void* ResolveMgoArray()`
- L67 `bool ReadFollowerPosition(void* follower, Vector& out)`
- L96 `void* ReadFollowerModelGob(void* follower)`
- L107 `bool ReadGobBoundingBox(void* gob, Vector& mn, Vector& mx)`
- L126 `bool ReadGobPartBoundingBox(void* gob, const char* partName, Vector& mn, Vector& mx)`
- L157 `void* CallAsCast(void* obj, size_t vtableSlotOffset)`
- L172 `bool ReadOffsetVector(void* player, Vector& out)`
- L184 `void WriteOffsetVector(void* player, const Vector& v)`
- L194 `float MagnetGain(float t, const MagnetParams& p)`
- L200 `float MagnetStep(float offsetVal, float mappedErr, float gain, const MagnetParams& p)`

# engine_area_internal.h (34 lines)

Source: `patches/Accessibility/engine_area_internal.h`

Internal to the engine_area module (engine_area*.cpp); no other file includes it. The public interface is engine_area.h. Here: what the files share.

## Declarations (in source order)

- L20 `bool IsSentinelHandle(uint32_t handle)`: engine_area.cpp
- L23 `bool TryReadLocString(void* base, size_t locStringOffset, char* outBuf, size_t bufSize)`: engine_area_names.cpp
- L24 `bool TryReadTag(void* obj, char* outBuf, size_t bufSize)`
- L25 `void AppendCommaSeparated(char* outBuf, size_t bufSize, const char* text)`
- L28 `void BuildDoorSuffix(void* serverDoor, char* outBuf, size_t bufSize)`: engine_area_doors.cpp
- L31 `void AppendLockNeed(void* obj, char* outBuf, size_t bufSize)`: engine_area_locks.cpp

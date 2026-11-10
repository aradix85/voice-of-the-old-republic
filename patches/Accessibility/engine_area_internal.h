// Internal to the engine_area module (engine_area*.cpp); no other file includes
// it. The public interface is engine_area.h. Here: what the files share.

#pragma once

#include "engine_area.h"

#include <windows.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>


namespace acc::engine {

// Helpers shared across the files, by the file that defines them.

// engine_area.cpp
bool IsSentinelHandle(uint32_t handle);

// engine_area_names.cpp
bool TryReadLocString(void* base, size_t locStringOffset, char* outBuf, size_t bufSize);
void AppendCommaSeparated(char* outBuf, size_t bufSize, const char* text);

// engine_area_doors.cpp
void BuildDoorSuffix(void* serverDoor, char* outBuf, size_t bufSize);

// engine_area_locks.cpp
void AppendLockNeed(void* obj, char* outBuf, size_t bufSize);

}  // namespace acc::engine

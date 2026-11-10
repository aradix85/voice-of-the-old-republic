// Internal to the item reads (engine_reads_items*.cpp); no other file includes
// it. The public interface is engine_reads.h. Here: what the files share.

#pragma once

#include "engine_reads.h"

#include "engine_app.h"     // GetServerApp

#include <windows.h>
#include <cstdint>
#include <cstring>

namespace acc::engine {

// Helpers shared across the files, by the file that defines them.

// engine_reads_items.cpp
void* ResolveItemFromServerHandle(uint32_t serverHandle);
bool ReadLocStringOtherLanguage(void* base, size_t locStringOffset, char* outBuf,
                                size_t bufSize);

// engine_reads_items_desc.cpp
bool ReadItemPropertyDescription(void* item, char* outBuf, size_t bufSize);

}  // namespace acc::engine

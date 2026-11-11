#include "engine_options.h"

#include <windows.h>
#include <cstdint>

#include "engine_app.h"     // GetClientAppInternal
#include "engine_reads.h"

namespace acc::engine {

namespace {

// Null at any null link or fault.
void* GetClientOptions() {
    void* internal = GetClientAppInternal();
    return ReadPtrOrNull(internal, kClientAppOptionsOffset);
}

}  // namespace

bool GetMouseLook(bool& out) {
    uint32_t bits = 0;
    if (!TryReadU32(GetClientOptions(), kClientOptionsBitFieldOffset, &bits)) return false;
    out = (bits & kClientOptionsMouseLookMask) != 0;
    return true;
}

bool GetActionMenuAutoPause(bool& out) {
    uint32_t bits = 0;
    if (!TryReadU32(GetClientOptions(), kClientOptionsAutoPauseFlagsOffset, &bits)) {
        return false;
    }
    out = (bits & kClientOptionsActionMenuAutoPauseMask) != 0;
    return true;
}

}  // namespace acc::engine

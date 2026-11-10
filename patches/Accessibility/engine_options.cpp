#include "engine_options.h"

#include <windows.h>
#include <cstdint>

#include "engine_app.h"     // GetClientAppInternal

namespace acc::engine {

namespace {

// Null at any null link or fault.
void* GetClientOptions() {
    void* internal = GetClientAppInternal();
    if (!internal) return nullptr;
    __try {
        return *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(internal) +
            kClientAppOptionsOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

}  // namespace

bool GetMouseLook(bool& out) {
    void* options = GetClientOptions();
    if (!options) return false;
    __try {
        unsigned int bits = *reinterpret_cast<unsigned int*>(
            reinterpret_cast<unsigned char*>(options) +
            kClientOptionsBitFieldOffset);
        out = (bits & kClientOptionsMouseLookMask) != 0;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool GetActionMenuAutoPause(bool& out) {
    void* options = GetClientOptions();
    if (!options) return false;
    __try {
        unsigned int bits = *reinterpret_cast<unsigned int*>(
            reinterpret_cast<unsigned char*>(options) +
            kClientOptionsAutoPauseFlagsOffset);
        out = (bits & kClientOptionsActionMenuAutoPauseMask) != 0;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

}  // namespace acc::engine

#include "engine_game.h"

#include <windows.h>

#include <cstdint>

namespace acc::game {

namespace {

// PE link timestamps of the two supported executables.
constexpr uint32_t kTsKotor1 = 0x402BC2D9;  // 2004-02-12 18:15:53Z, version 1.0.3
constexpr uint32_t kTsKotor2 = 0x5603005D;  // 2015-09-23 19:41:17Z, Aspyr's rebuild

uint32_t ReadGameLinkTimestamp() {
    // GetModuleHandle(nullptr) is the EXE, not this DLL.
    auto base = reinterpret_cast<const uint8_t*>(GetModuleHandleW(nullptr));
    if (!base) return 0;

    // This runs during static initialisation, where a fault is painful to
    // diagnose, and before anything has validated the image: every step is
    // checked.
    if (base[0] != 'M' || base[1] != 'Z') return 0;
    int32_t lfanew = *reinterpret_cast<const int32_t*>(base + 0x3C);
    if (lfanew <= 0 || lfanew > 0x1000) return 0;
    const uint8_t* pe = base + lfanew;
    if (pe[0] != 'P' || pe[1] != 'E' || pe[2] != 0 || pe[3] != 0) return 0;
    return *reinterpret_cast<const uint32_t*>(pe + 8);
}

Title DetectTitle() {
    switch (ReadGameLinkTimestamp()) {
        case kTsKotor1: return Title::Kotor1;
        case kTsKotor2: return Title::Kotor2;
        default:        return Title::Unknown;
    }
}

}  // namespace

// Function-local static: initialised on first use, with no dependency on any
// other global's initialisation order.
Title CurrentTitle() {
    static const Title t = DetectTitle();
    return t;
}

const char* TitleName() {
    switch (CurrentTitle()) {
        case Title::Kotor1: return "kotor1";
        case Title::Kotor2: return "kotor2";
        default:            return "unknown";
    }
}

}  // namespace acc::game

#include "engine_compass.h"

#include <cmath>

#include "engine_offsets_types.h"

namespace acc::engine {

namespace {

constexpr int kSectorCount = 8;

}  // namespace

float EngineYawToCompass(float engineYawDeg) {
    float c = std::fmod(90.0f - engineYawDeg + 360.0f, 360.0f);
    if (c < 0.0f) c += 360.0f;
    return c;
}

int CompassToSector(float compassDeg) {
    float adj = std::fmod(compassDeg + kHalfSector + 360.0f, 360.0f);
    int s = static_cast<int>(adj / kSectorSize);
    if (s < 0)              s = 0;
    if (s >= kSectorCount)  s = kSectorCount - 1;
    return s;
}

float VectorEngineYaw(float dx, float dy) {
    const float yaw = std::atan2(dy, dx) * kRadToDeg;
    return yaw < 0.0f ? yaw + 360.0f : yaw;
}

float VectorCompass(float dx, float dy) {
    return EngineYawToCompass(VectorEngineYaw(dx, dy));
}

int VectorSector(float dx, float dy) {
    return CompassToSector(VectorCompass(dx, dy));
}

int ClockPosition(float playerYawDeg, float dx, float dy) {
    float relCcw = VectorEngineYaw(dx, dy) - playerYawDeg;
    float clockDeg = -relCcw;  // flip to clockwise from +ahead
    while (clockDeg < 0.0f)    clockDeg += 360.0f;
    while (clockDeg >= 360.0f) clockDeg -= 360.0f;
    int hour = static_cast<int>(clockDeg / 30.0f + 0.5f) % 12;
    return hour == 0 ? 12 : hour;
}

acc::strings::Id SectorString(int sector) {
    using S = acc::strings::Id;
    switch (sector) {
        case 0: return S::DirNorth;
        case 1: return S::DirNortheast;
        case 2: return S::DirEast;
        case 3: return S::DirSoutheast;
        case 4: return S::DirSouth;
        case 5: return S::DirSouthwest;
        case 6: return S::DirWest;
        case 7: return S::DirNorthwest;
    }
    return S::DirNorth;  // unreachable
}

}  // namespace acc::engine

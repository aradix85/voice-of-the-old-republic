// The area map's pixel transform: world position to map pixel and back, as
// CSWSAreaMap::GetMapPixelFromWorldCoord computes it. Inverted inline, so
// the cursor's hot path makes no engine call.

#include "map_ui_cursor_internal.h"

#include "engine_area.h"

namespace acc::map_ui_cursor {

namespace {

// The CSWSAreaMap transform fields (engine_area.h). False on a read fault;
// all of them are needed.
bool ReadAreaMapTransform(void* areaMap,
                          uint32_t& outOrientation,
                          float& outWuPerPx, float& outWuPerPy,
                          float& outOriginX, float& outOriginY) {
    if (!areaMap) return false;
    __try {
        auto* b = reinterpret_cast<unsigned char*>(areaMap);
        outOrientation = *reinterpret_cast<uint32_t*>(b + kAreaMapNorthAxisOffset);
        outWuPerPx     = *reinterpret_cast<float*>   (b + kAreaMapWorldPerPxXOffset);
        outWuPerPy     = *reinterpret_cast<float*>   (b + kAreaMapWorldPerPxYOffset);
        outOriginX     = *reinterpret_cast<float*>   (b + kAreaMapWorldOriginXOffset);
        outOriginY     = *reinterpret_cast<float*>   (b + kAreaMapWorldOriginYOffset);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// The engine's forward rotation per orientation, as
// CSWSAreaMap::GetMapPixelFromWorldCoord decompiles:
//   case 1: (xw, yw) -> (-xw, -yw)
//   case 2: (xw, yw) -> (yw,  -xw)
//   case 3: (xw, yw) -> (-yw,  xw)
void ApplyOrientationForward(uint32_t orient, float& xw, float& yw) {
    float ox = xw, oy = yw;
    switch (orient) {
        case 0: /* identity */                       break;
        case 1: xw = -ox; yw = -oy;                  break;
        case 2: xw =  oy; yw = -ox;                  break;
        case 3: xw = -oy; yw =  ox;                  break;
        default: /* unknown — leave unmolested */    break;
    }
}

// Undo it.
void ApplyOrientationInverse(uint32_t orient, float& xw, float& yw) {
    float ox = xw, oy = yw;
    switch (orient) {
        case 0: /* identity */                       break;
        case 1: xw = -ox; yw = -oy;                  break;  // self-inverse
        case 2: xw = -oy; yw =  ox;                  break;  // inverse of (y,-x) = (-y, x)
        case 3: xw =  oy; yw = -ox;                  break;  // inverse of (-y, x) = (y, -x)
        default: /* unknown */                       break;
    }
}

}  // namespace

bool WorldToPixel(void* areaMap, const Vector& world, float& outPx, float& outPy) {
    uint32_t orient; float wuPx, wuPy, ox, oy;
    if (!ReadAreaMapTransform(areaMap, orient, wuPx, wuPy, ox, oy)) return false;
    if (wuPx == 0.0f || wuPy == 0.0f) return false;
    float xw = world.x, yw = world.y;
    ApplyOrientationForward(orient, xw, yw);
    outPx = (xw - ox) / wuPx + 0.5f;
    outPy = (yw - oy) / wuPy + 0.5f;
    return true;
}

bool PixelToWorld(void* areaMap, float px, float py, float zSeed, Vector& outWorld) {
    uint32_t orient; float wuPx, wuPy, ox, oy;
    if (!ReadAreaMapTransform(areaMap, orient, wuPx, wuPy, ox, oy)) return false;
    float xw = px * wuPx + ox;
    float yw = py * wuPy + oy;
    ApplyOrientationInverse(orient, xw, yw);
    outWorld.x = xw;
    outWorld.y = yw;
    outWorld.z = zSeed;
    return true;
}

}  // namespace acc::map_ui_cursor

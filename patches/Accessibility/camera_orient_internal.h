// Shared between the camera_orient files: the angle helpers, the camera
// heading read, and the two turn drives (camera_orient_turn.cpp) that Tick
// starts and ticks.

#pragma once

#include <windows.h>

#include "camera_orient.h"

namespace acc::camera_orient {

// Single rotation at a time. Rate-based predictive release: each tick
// samples yaw + timestamp so the next tick projects time-to-target and
// releases the key ahead of arrival to absorb input-pipeline latency.
// Without prediction the engine keeps rotating ~30-50ms after our keyup.
struct Rotation {
    bool   active             = false;
    WORD   holdScan           = 0;       // DIK scancode of the held turn key
    char   debugKey           = 0;       // 'L' / 'R' for log readability
    float  targetEngineYawRad = 0.0f;
    float  initialAbsDeltaRad = 0.0f;    // overshoot detection
    DWORD  startedMs          = 0;
    float  prevYawRad         = 0.0f;
    DWORD  prevTickMs         = 0;
    bool   haveRateSample     = false;
};
extern Rotation g_rot;

struct Snap {
    bool        active             = false;
    bool        taken              = false;  // the camera consumed the request
    float       targetEngineYawRad = 0.0f;
    DWORD       startedMs          = 0;
    const char* mode               = "";     // log label, a literal
};
extern Snap g_snap;

// Fallback when no rate sample yet or rate too small to project. 2.9°.
constexpr float kFallbackArrivalRad = 0.05f;

constexpr float kPi      = 3.14159265358979323846f;
constexpr float kTwoPi   = 6.28318530717958647692f;
constexpr float kRadToDeg = 57.29577951308232f;
constexpr float kDegToRad = 0.017453292519943295f;

inline float NormaliseRad(float r) {
    while (r >  kPi)  r -= kTwoPi;
    while (r <= -kPi) r += kTwoPi;
    return r;
}

bool ReadCurrentEngineYawRad(float& out);
void LogLanding();
bool ArmKeyTurn(float targetEngineYawRad, float curYawRad, const char* mode);
void TickKeyTurn(void* camera);
bool StartSnap(float targetEngineYawRad, float curYawRad, const char* mode);
void TickSnap(void* camera);

}  // namespace acc::camera_orient

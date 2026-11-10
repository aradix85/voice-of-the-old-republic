// The spoken-direction frame — see viewpoint.h.

#include "viewpoint.h"

#include "camera_announce.h"
#include "engine_player.h"

namespace acc::viewpoint {

bool YawDegrees(float& out) {
    float cameraYaw = 0.0f;
    if (acc::camera_announce::TryGetCameraEngineYawDegrees(cameraYaw)) {
        out = cameraYaw;
        return true;
    }
    return acc::engine::GetPlayerYawDegrees(out);
}

bool Get(Vector& outPos, float& outYawDegrees) {
    Vector pos{};
    float  yaw = 0.0f;
    if (!acc::engine::GetPlayerPosition(pos) || !YawDegrees(yaw)) return false;
    outPos        = pos;
    outYawDegrees = yaw;
    return true;
}

}  // namespace acc::viewpoint

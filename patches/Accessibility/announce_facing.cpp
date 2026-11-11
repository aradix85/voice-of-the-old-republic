#include "announce_facing.h"

#include <cmath>
#include <cstdio>
#include <string>

#include "camera_announce.h"
#include "engine_area.h"
#include "engine_compass.h"
#include "engine_panels.h"
#include "engine_player.h"
#include "hotkeys.h"
#include "log.h"
#include "strfmt.h"
#include "strings.h"
#include "speech.h"
#include "transitions.h"
#include "room_topology.h"

namespace acc::announce_facing {

namespace {

int CompassDegreesFromEngineYaw(float engineYaw) {
    float compass = acc::engine::EngineYawToCompass(engineYaw);
    int degrees = static_cast<int>(std::floor(compass + 0.5f)) % 360;
    if (degrees < 0) degrees += 360;
    return degrees;
}

// Camera facing in engine yaw (0° = +X, CCW+). Prefers camera_announce's
// cached value; falls back to deriving from camera→player vector when
// announce hasn't anchored.
bool ReadCameraEngineYawDegrees(float& out) {
    if (acc::camera_announce::TryGetCameraEngineYawDegrees(out)) return true;
    Vector cam, player;
    if (!acc::engine::GetCameraPosition(cam)) return false;
    if (!acc::engine::GetPlayerPosition(player)) return false;
    float dx = player.x - cam.x;
    float dy = player.y - cam.y;
    if (std::fabs(dx) < 1e-3f && std::fabs(dy) < 1e-3f) return false;
    out = acc::engine::VectorEngineYaw(dx, dy);
    return true;
}

const char* SectorWord(int compassDegrees) {
    int sector = acc::engine::CompassToSector(static_cast<float>(compassDegrees));
    return acc::strings::Get(acc::engine::SectorString(sector));
}

// Perceptual-region label at the player's position. False when no
// labelled cluster (graph not built, out-of-snap, or open-area-only — too
// vague to add value). Cluster source matches transitions.cpp so this
// readout agrees with the last in-world cluster announcement.
bool ResolveClusterLabelForPlayer(std::string& outBuf) {
    outBuf.clear();

    Vector pos;
    if (!acc::engine::GetPlayerPosition(pos)) return false;
    void* area = acc::engine::GetCurrentArea();
    if (!area) return false;

    std::string buf;
    int sig = 0;
    int cid = acc::room_topology::kClusterIdNone;
    if (!acc::room_topology::LookupAt(area, pos, buf, sig, cid,
                                      /*allowDiagLog=*/true,
                                      /*requireWallReachable=*/true,
                                      /*zTrusted=*/true)) {
        return false;
    }
    if (cid == acc::room_topology::kClusterIdNone ||
        cid == acc::room_topology::kClusterIdOpenArea ||
        buf.empty()) {
        return false;
    }
    outBuf = buf;
    return true;
}

void AnnounceWorldFacing() {
    float engineYaw = 0.0f;
    if (!ReadCameraEngineYawDegrees(engineYaw)) {
        // Mid-transient (spawn / area-load / chain degenerate).
        acclog::Write("AnnounceFacing", "camera yaw unavailable, skipping");
        return;
    }
    int degrees    = CompassDegreesFromEngineYaw(engineYaw);
    const char* sector = SectorWord(degrees);

    std::string clusterBuf;
    bool haveCluster = ResolveClusterLabelForPlayer(clusterBuf);

    std::string facing = acc::strfmt::Format(
        acc::strings::Get(acc::strings::Id::FmtFacingWorld), sector);
    // The area on its own line, so the heading does not run into the area's
    // own direction words.
    if (haveCluster) facing += "\n" + clusterBuf;
    acc::speech::Speak(facing.c_str(), /*interrupt=*/true);
    acclog::Write("AnnounceFacing", "world -> [%s] (camYaw=%.1f, deg=%d, sector=\"%s\")",
                  facing.c_str(), engineYaw, degrees, sector);
}

// Three-tier room name lookup matching transitions::Tick:
// landmark → friendly room name → synthetic "Room N".
bool ResolveRoomNameForPlayer(char* outBuf, size_t bufSize) {
    if (!outBuf || bufSize < 2) return false;
    outBuf[0] = '\0';

    Vector pos;
    if (!acc::engine::GetPlayerPosition(pos)) return false;
    void* area = acc::engine::GetCurrentArea();
    if (!area) return false;

    constexpr float kLandmarkRangeM = 15.0f;
    char   landmarkBuf[128] = {0};
    Vector landmarkPos;
    if (acc::transitions::FindLandmarkNear(
            pos, kLandmarkRangeM,
            landmarkBuf, sizeof(landmarkBuf), landmarkPos) &&
        landmarkBuf[0] != '\0') {
        std::snprintf(outBuf, bufSize, "%s", landmarkBuf);
        return true;
    }

    int roomIdx = -1;
    acc::engine::GetRoomAtIndexed(area, pos, roomIdx);
    if (roomIdx < 0) return false;

    // Skip resref-style noise like "m02_03e".
    char roomBuf[128] = {0};
    if (acc::engine::GetRoomDisplayName(area, roomIdx,
                                        roomBuf, sizeof(roomBuf)) &&
        roomBuf[0] != '\0' &&
        !acc::transitions::IsResrefStyleRoomName(roomBuf)) {
        std::snprintf(outBuf, bufSize, "%s", roomBuf);
        return true;
    }
    std::snprintf(outBuf, bufSize,
                  acc::strings::Get(acc::strings::Id::FmtTransitionRoomIndex),
                  roomIdx);
    return true;
}

void AnnounceMapFacing() {
    // Either failure falls back to world-frame so the key never feels eaten.
    float camYawDeg = 0.0f;
    if (!ReadCameraEngineYawDegrees(camYawDeg)) {
        acclog::Write("AnnounceFacing",
            "map: camera facing unavailable; falling back to world");
        AnnounceWorldFacing();
        return;
    }
    float yawRad = camYawDeg * kDegToRad;
    Vector facing;
    facing.x = std::cos(yawRad);
    facing.y = std::sin(yawRad);
    facing.z = 0.0f;
    void* areaMap = acc::engine::GetAreaMap();
    if (!areaMap) {
        acclog::Write("AnnounceFacing",
            "map: areaMap unavailable; falling back to world");
        AnnounceWorldFacing();
        return;
    }

    float mapYawCCW = 0.0f;
    if (!acc::engine::GetMapRotateCCWFromWorldOrientation(
            areaMap, facing, mapYawCCW)) {
        acclog::Write("AnnounceFacing",
            "map: GetMapRotateCCW failed; falling back to world");
        AnnounceWorldFacing();
        return;
    }

    // Engine returns CCW-from-+X in map space, same convention as engine
    // yaw — EngineYawToCompass gives CW-from-North matching the map arrow.
    int degrees = CompassDegreesFromEngineYaw(mapYawCCW);
    const char* sector = SectorWord(degrees);

    char roomBuf[160];
    bool haveRoom = ResolveRoomNameForPlayer(roomBuf, sizeof(roomBuf));

    std::string line = acc::strfmt::Format(
        acc::strings::Get(acc::strings::Id::FmtFacingMap), degrees, sector);
    // The room on its own line, as in the world readout.
    if (haveRoom) {
        line += "\n";
        line += roomBuf;
    }
    acc::speech::Speak(line.c_str(), /*interrupt=*/true);
    acclog::Write("AnnounceFacing", "map -> [%s] (camYaw=%.1f, mapYawCCW=%.1f, deg=%d)",
                  line.c_str(), camYawDeg, mapYawCCW, degrees);
}

}  // namespace

void PollWin32() {
    if (!acc::hotkeys::Pressed(acc::hotkeys::Action::AnnounceDegrees)) return;

    Vector playerPos;
    if (!acc::engine::GetPlayerPosition(playerPos)) return;

    if (acc::engine::HasActiveMapPanel()) {
        AnnounceMapFacing();
    } else {
        AnnounceWorldFacing();
    }
}

}  // namespace acc::announce_facing

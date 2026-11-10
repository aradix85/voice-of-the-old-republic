// The free-directions probe — see audio_stuck_probe.h.

#include "audio_stuck_probe.h"

#include <string>

#include "engine_area.h"
#include "log.h"
#include "spatial_wall_surfaces.h"
#include "speech.h"
#include "strings.h"
#include "stuck_detect.h"

namespace acc::audio::stuck_probe {

namespace {

using acc::strings::Get;
using acc::strings::Id;

// 1.5 m is about two character lengths; 0.5 m of body clearance covers a
// creature capsule (~0.4 m radius) plus margin.
constexpr float kProbeMetres         = 1.5f;
constexpr float kBodyClearanceMetres = 0.5f;
constexpr float kSelfEpsilonSq       = 0.04f;  // within 0.2 m: the leader
constexpr int   kMaxNearbyBodies     = 32;

// +X east, +Y north.
constexpr float kDiag = 0.70710678f;
struct ProbeDir {
    float       x, y;
    Id          word;
    const char* tag;  // for the log
};
const ProbeDir kProbeDirs[8] = {
    {0.0f, 1.0f, Id::DirNorth, "N"},
    {kDiag, kDiag, Id::DirNortheast, "NE"},
    {1.0f, 0.0f, Id::DirEast, "E"},
    {kDiag, -kDiag, Id::DirSoutheast, "SE"},
    {0.0f, -1.0f, Id::DirSouth, "S"},
    {-kDiag, -kDiag, Id::DirSouthwest, "SW"},
    {-1.0f, 0.0f, Id::DirWest, "W"},
    {-kDiag, kDiag, Id::DirNorthwest, "NW"},
};

NoProgressTrigger g_noProgress;
CirclingTrigger   g_circling;

// Creatures and placeables within the probe's reach; the leader is left out
// by position. Returns the count written.
int CollectNearbyBodies(const Vector& pos, Vector* bodies, int cap) {
    using acc::engine::GameObjectKind;
    void* area = acc::engine::GetCurrentArea();
    if (!area) return 0;
    const float reach = kProbeMetres + kBodyClearanceMetres;
    int count = 0;
    acc::engine::AreaObjectIterator it(area);
    for (void* obj = it.Next(); obj && count < cap; obj = it.Next()) {
        const int kind = acc::engine::GetObjectKind(obj);
        if (kind != static_cast<int>(GameObjectKind::Creature) &&
            kind != static_cast<int>(GameObjectKind::Placeable)) {
            continue;
        }
        Vector p;
        if (!acc::engine::GetObjectPosition(obj, p)) continue;
        const float dsq = DistXYSq(p, pos);
        if (dsq >= kSelfEpsilonSq && dsq <= reach * reach) bodies[count++] = p;
    }
    return count;
}

// What blocks the ray pos -> end: "wall", "body", or nullptr when it is free.
const char* ProbeBlocker(const Vector& pos, const Vector& end,
                         const acc::engine::WallEdge* walls, int wallCount,
                         const Vector* bodies, int bodyCount) {
    Vector hit;
    if (acc::engine::SegmentCrossesWalkmesh(walls, wallCount, pos, end, hit)) {
        return "wall";
    }
    for (int i = 0; i < bodyCount; ++i) {
        if (DistXYToSegmentSq(bodies[i], pos, end) <
            kBodyClearanceMetres * kBodyClearanceMetres) {
            return "body";
        }
    }
    return nullptr;
}

void RunProbe(const Vector& pos) {
    const acc::engine::WallEdge* walls = nullptr;
    int wallCount = 0;
    if (!acc::spatial::wall_surfaces::GetCachedWalls(walls, wallCount)) wallCount = 0;
    Vector bodies[kMaxNearbyBodies];
    const int bodyCount = CollectNearbyBodies(pos, bodies, kMaxNearbyBodies);

    std::string freeDirs;
    std::string detail;
    for (const ProbeDir& d : kProbeDirs) {
        const Vector end{pos.x + d.x * kProbeMetres, pos.y + d.y * kProbeMetres, pos.z};
        const char* blocker = ProbeBlocker(pos, end, walls, wallCount, bodies, bodyCount);
        detail += std::string(" ") + d.tag + "=" + (blocker ? blocker : "free");
        if (blocker) continue;
        if (!freeDirs.empty()) freeDirs += ", ";
        freeDirs += Get(d.word);
    }
    const std::string line =
        freeDirs.empty() ? std::string(Get(Id::StuckAllBlocked))
                         : std::string(Get(Id::StuckFreeDirsPrefix)) + ": " + freeDirs;
    acclog::Write("StuckProbe", "fire pos=(%.2f,%.2f) walls=%d bodies=%d%s -> [%s]",
                  pos.x, pos.y, wallCount, bodyCount, detail.c_str(), line.c_str());
    acc::speech::Speak(line.c_str(), /*interrupt=*/true);
}

}  // namespace

void Tick(const Vector& pos, uint64_t now_ms, bool walking) {
    if (g_noProgress.Update(pos, now_ms, walking)) RunProbe(pos);
    if (g_circling.Update(pos, now_ms, walking)) {
        const float path = g_circling.PathMetres();
        const float net = g_circling.NetMetres();
        acclog::Write("StuckProbe",
                      "circling detected: path=%.2fm net=%.2fm ratio=%.2f window_ms=%llu",
                      path, net, net > 0.01f ? path / net : path,
                      static_cast<unsigned long long>(g_circling.SpanMs()));
        RunProbe(pos);
    }
}

void Reset() {
    g_noProgress.Reset();
    g_circling.Reset();
}

}  // namespace acc::audio::stuck_probe

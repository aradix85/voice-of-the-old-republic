// The region graph and walking routes: shortest walks with and without
// locked doors, the exit to take, route words, the two sides of a door,
// doorway slivers of area doors, reachability flags for the cycles, /rooms.

#include "exit_memory_internal.h"

#include "engine_area.h"
#include "engine_compass.h"
#include "engine_player.h"     // position, yaw, party, player creature
#include "log.h"
#include "shortest_paths.h"
#include "strings.h"

namespace acc::exit_memory {

namespace {

// Walking distances are spoken rounded to this many metres.
constexpr int kWalkRoundM = 5;

using acc::paths::Adjacency;
using acc::paths::ShortestPaths;

// TransitionSliverOwner state: the last answer, reused for a short while
// (the cluster tick asks every frame).
struct SliverCache {
    void*  area   = nullptr;
    int    region = kClusterIdNone;
    int    owner  = kClusterIdNone;
    Vector centre = {0.0f, 0.0f, 0.0f};
    DWORD  atMs   = 0;
};
SliverCache g_sliver;
int         g_sliverLogged = kClusterIdNone;
constexpr DWORD kSliverCacheMs    = 2000;
// A lift cab or a doorway; anything wider is a real place.
constexpr float kSliverMaxExtentM = 10.0f;

}  // namespace

bool BuildGraph(void* area, const Vector& player, Graph& g) {
    g = Graph{};
    g.area   = area;
    g.player = player;
    if (!area || !acc::room_topology::ListClusters(area, g.nodes) ||
        g.nodes.empty()) {
        return false;
    }
    std::string label;
    int sig = 0;
    acc::room_topology::LookupAt(area, player, label, sig, g.underPlayer,
                                 /*allowDiagLog=*/false,
                                 /*requireWallReachable=*/true,
                                 /*zTrusted=*/true, &g.exits);
    g.start = (area == g_area && g_current >= 0) ? g_current : g.underPlayer;
    // In the doorway sliver of an area door the region in front of it is
    // where the player is (and ExitToward then names no exit of the sliver).
    g.start = TransitionSliverOwner(area, g.start, nullptr);

    acc::room_topology::GetClusterLinks(area, g.links);
    const std::vector<Vector> lockedDoors = LockedDoorPositions(area);
    Adjacency open(g.nodes.size()), all(g.nodes.size());
    for (const ClusterLink& l : g.links) {
        if (l.a == g.start) g.linked.insert(l.b);
        else if (l.b == g.start) g.linked.insert(l.a);
        const int a = g.IndexOf(l.a);
        const int b = g.IndexOf(l.b);
        if (a < 0 || b < 0) continue;
        const float w =
            (std::max)(0.5f, DistXY(g.nodes[a].centre, g.nodes[b].centre));
        all[a].emplace_back(b, w);
        all[b].emplace_back(a, w);
        if (l.viaDoor && NearAny(l.doorPos, lockedDoors, kDoorMatchM)) continue;
        open[a].emplace_back(b, w);
        open[b].emplace_back(a, w);
    }
    g.si = g.IndexOf(g.start);
    ShortestPaths(open, g.si, g.distOpen, g.hopOpen);
    ShortestPaths(all, g.si, g.distAll, g.hopAll);
    return true;
}

// The exit of the player's region that leads into region index `hop`,
// numbered like the cycles ("Door 2 East"). Empty when the player is not
// standing in the start region or no exit points that way.
std::string ExitToward(const Graph& g, int hop, Vector* outPos) {
    if (hop < 0 || hop >= static_cast<int>(g.nodes.size()) ||
        g.underPlayer != g.start || g.exits.empty()) {
        return std::string();
    }
    ClusterInfo here;
    const bool haveCentre =
        acc::room_topology::GetClusterInfo(g.area, g.start, here);
    const int hopId = g.nodes[hop].id;
    for (const ExitMark& mark : g.exits) {
        if (mark.text.empty()) continue;
        const Vector from = haveCentre ? here.centre : mark.pos;
        if (RegionBehind(g.area, g.start, g.linked, from, mark.pos) != hopId) {
            continue;
        }
        ExitMark e = mark;
        NumberDoorExit(e, NumberedDoors());
        if (outPos) *outPos = e.pos;
        return e.text;
    }
    return std::string();
}

// The way from the player to `targetPos`, which lies in region index `t`.
// The walk is player -> centre of the first region on the way -> centre to
// centre -> the target, never less than the straight line.
Route RouteToIndex(const Graph& g, int t, const Vector& targetPos) {
    Route r;
    if (g.si < 0 || t < 0 || t >= static_cast<int>(g.nodes.size())) return r;
    if (t == g.si) {
        r.kind = Route::Here;
        return r;
    }
    if (g.distOpen[t] < kFar && g.hopOpen[t] >= 0) {
        const int hop = g.hopOpen[t];
        r.kind = Route::Open;
        r.via  = ExitToward(g, hop, nullptr);
        float m = DistXY(g.player, g.nodes[hop].centre) +
                  (g.distOpen[t] - g.distOpen[hop]) +
                  DistXY(g.nodes[t].centre, targetPos);
        m = (std::max)(m, DistXY(g.player, targetPos));
        r.walkM = acc::paths::RoundWalk(m, kWalkRoundM);
        return r;
    }
    r.kind = g.distAll[t] < kFar ? Route::Locked : Route::Unreachable;
    return r;
}

Route RouteTo(const Graph& g, const Vector& targetPos) {
    return RouteToIndex(g, g.IndexOf(RegionAt(g.area, targetPos)), targetPos);
}

std::string RouteText(const Route& r) {
    using acc::strings::Id;
    char buf[320];
    switch (r.kind) {
        case Route::Open:
            if (!r.via.empty()) {
                std::snprintf(buf, sizeof(buf), acc::strings::Get(Id::FmtRouteVia),
                              r.via.c_str(), r.walkM);
            } else {
                std::snprintf(buf, sizeof(buf), acc::strings::Get(Id::FmtRouteWalk),
                              r.walkM);
            }
            return buf;
        case Route::Locked:
            return acc::strings::Get(Id::RouteLocked);
        default:
            // Unreachable stays quiet: a region the graph has no link to
            // may still be reached by a lift or a scene the graph cannot see.
            return std::string();
    }
}

// Where a door leads, from the player's side: the compass direction through
// it and the region beyond ("leads West, to a corridor, visited"; "leads
// South, back"). `outNear` gets the graph index of the region on the
// player's side, -1 when unknown. Empty when the graph joins nothing
// through the door (area transitions, set dressing).
// The two regions a door joins, seen from the player: `nearId` on the
// player's side, `farId` beyond. False when the graph joins nothing through
// it. Reads no cycle listings (the cycles call into here).
bool DoorSides(const Graph& g, const Vector& doorPos, int& nearId, int& farId) {
    nearId = kClusterIdNone;
    farId  = kClusterIdNone;

    // The regions the door joins, from the links recorded through it.
    std::vector<int> sides;
    for (const ClusterLink& l : g.links) {
        if (!l.viaDoor || DistXY(l.doorPos, doorPos) > kDoorMatchM) continue;
        if (std::find(sides.begin(), sides.end(), l.a) == sides.end()) {
            sides.push_back(l.a);
        }
        if (std::find(sides.begin(), sides.end(), l.b) == sides.end()) {
            sides.push_back(l.b);
        }
    }
    auto walkTo = [&g](int id) {
        const int i = g.IndexOf(id);
        return (i >= 0 && i < static_cast<int>(g.distAll.size())) ? g.distAll[i]
                                                                  : kFar;
    };
    if (sides.size() >= 2) {
        // Near: the player's region when the door is one of its own, else
        // the side reached first. Far: the side reached last.
        float bestNear = kFar;
        for (int id : sides) {
            const float d = (id == g.start) ? -1.0f : walkTo(id);
            if (nearId < 0 || d < bestNear) {
                bestNear = d;
                nearId = id;
            }
        }
        float bestFar = -1.0f;
        for (int id : sides) {
            if (id == nearId) continue;
            const float d = walkTo(id);
            if (farId < 0 || d > bestFar) {
                bestFar = d;
                farId = id;
            }
        }
    } else {
        // No link through it: probe past the door on the line from the
        // player, and accept only a region linked to the near side.
        const float len = DistXY(g.player, doorPos);
        if (len < 0.5f) return false;
        const float ux = (doorPos.x - g.player.x) / len;
        const float uy = (doorPos.y - g.player.y) / len;
        const Vector before{doorPos.x - ux * 2.0f, doorPos.y - uy * 2.0f,
                            doorPos.z};
        nearId = RegionAt(g.area, before);
        if (nearId < 0) return false;
        std::set<int> nearLinked;
        for (const ClusterLink& l : g.links) {
            if (l.a == nearId) nearLinked.insert(l.b);
            else if (l.b == nearId) nearLinked.insert(l.a);
        }
        farId = RegionBehind(g.area, nearId, nearLinked, g.player, doorPos);
        if (farId < 0 || !nearLinked.count(farId)) return false;
    }
    return nearId >= 0 && farId >= 0;
}

std::string DoorBeyond(const Graph& g, const Vector& doorPos, int* outNear) {
    using acc::strings::Id;
    if (outNear) *outNear = -1;
    int nearId = kClusterIdNone;
    int farId  = kClusterIdNone;
    if (!DoorSides(g, doorPos, nearId, farId)) return std::string();
    if (outNear) *outNear = g.IndexOf(nearId);

    // Direction: from the near region's centre out through the door, the
    // same way the region label words its exits ("Door East").
    float dx = 0.0f;
    float dy = 0.0f;
    ClusterInfo info;
    if (acc::room_topology::GetClusterInfo(g.area, nearId, info) &&
        DistXY(info.centre, doorPos) >= 1.0f) {
        dx = doorPos.x - info.centre.x;
        dy = doorPos.y - info.centre.y;
    } else if (acc::room_topology::GetClusterInfo(g.area, farId, info)) {
        dx = info.centre.x - doorPos.x;
        dy = info.centre.y - doorPos.y;
    }
    if (dx * dx + dy * dy < 0.25f) return std::string();
    const float yaw = std::atan2(dy, dx) * 57.29578f;
    const int sector =
        acc::engine::CompassToSector(acc::engine::EngineYawToCompass(yaw));
    char buf[320];
    std::snprintf(buf, sizeof(buf), acc::strings::Get(Id::FmtDoorLeads),
                  acc::strings::Get(acc::engine::SectorString(sector)));
    std::string out = buf;

    if (g.area == g_area && nearId == g.start && farId == g_previous) {
        out += ", ";
        out += acc::strings::Get(Id::ExitAnnoBack);
        return out;
    }
    const std::string dest =
        Destination(g.area, farId, LandmarkRegions(g.area), std::string());
    if (!dest.empty()) out += ", " + dest;
    if (g.area == g_area) {
        out += ", ";
        out += g_visited.count(farId) ? acc::strings::Get(Id::ExitAnnoVisited)
                                      : acc::strings::Get(Id::ExitAnnoNew);
    }
    // Visited: who and what is still there ("Matrik", "nothing left").
    const std::string behind = BehindDoorText(g.area, farId);
    if (!behind.empty()) out += ", " + behind;
    return out;
}

int TransitionSliverOwner(void* area, int region, Vector* outCentre) {
    if (!area || region < 0) return region;
    const DWORD now = GetTickCount();
    if (g_sliver.area == area && g_sliver.region == region &&
        now - g_sliver.atMs < kSliverCacheMs) {
        if (g_sliver.owner != region && outCentre) *outCentre = g_sliver.centre;
        return g_sliver.owner;
    }
    g_sliver.area   = area;
    g_sliver.region = region;
    g_sliver.owner  = region;
    g_sliver.atMs   = now;

    ClusterInfo info;
    if (!acc::room_topology::GetClusterInfo(area, region, info) ||
        info.extentM < 0.0f || info.extentM > kSliverMaxExtentM) {
        return region;
    }
    std::vector<ClusterLink> links;
    if (!acc::room_topology::GetClusterLinks(area, links)) return region;
    int   owner    = kClusterIdNone;
    void* viaDoor  = nullptr;
    for (const ClusterLink& l : links) {
        if (l.a != region && l.b != region) continue;
        const int other = (l.a == region) ? l.b : l.a;
        if (!l.viaDoor) return region;  // an open way: a place of its own
        void* door = DoorNear(area, l.doorPos);
        if (!door || !acc::engine::DoorHasTransitionDest(door)) return region;
        if (owner != kClusterIdNone && owner != other) return region;
        owner   = other;
        viaDoor = door;
    }
    if (owner == kClusterIdNone) return region;
    ClusterInfo oi;
    if (!acc::room_topology::GetClusterInfo(area, owner, oi)) return region;
    g_sliver.owner  = owner;
    g_sliver.centre = oi.centre;
    if (outCentre) *outCentre = oi.centre;
    if (g_sliverLogged != region) {
        g_sliverLogged = region;
        acclog::Write("ExitMemory", "region %d (extent %.1fm) is the doorway "
                      "sliver of area door %p; standing in it counts as "
                      "region %d", region, info.extentM, viaDoor, owner);
    }
    return owner;
}

bool UnreachableFlags(void* const* objs, const Vector* positions, int count,
                      bool* out) {
    if (!positions || !out || count <= 0) return false;
    Vector player;
    if (!acc::engine::GetPlayerPosition(player)) return false;
    Graph g;
    if (!BuildGraph(acc::engine::GetCurrentArea(), player, g) || g.si < 0) {
        return false;
    }
    for (int i = 0; i < count; ++i) {
        out[i] = false;
        int t = -1;
        void* obj = objs ? objs[i] : nullptr;
        if (obj && acc::filter::ObjectMatches(obj, CycleCategory::Door)) {
            // A door counts by the side the player would walk up to.
            int nearId = kClusterIdNone;
            int farId  = kClusterIdNone;
            if (DoorSides(g, positions[i], nearId, farId)) t = g.IndexOf(nearId);
        }
        if (t < 0) t = g.IndexOf(RegionAt(g.area, positions[i]));
        if (t < 0 || t == g.si) continue;
        out[i] = g.distOpen[t] >= kFar;
    }
    return true;
}

bool ListRegions(std::vector<RegionSummary>& out,
                 std::vector<std::pair<int, int>>& links) {
    out.clear();
    links.clear();
    Vector player;
    if (!acc::engine::GetPlayerPosition(player)) return false;
    void* area = acc::engine::GetCurrentArea();
    Graph g;
    if (!BuildGraph(area, player, g)) return false;
    for (size_t i = 0; i < g.nodes.size(); ++i) {
        RegionSummary r;
        r.id      = g.nodes[i].id;
        r.centre  = g.nodes[i].centre;
        switch (g.nodes[i].kind) {
            case acc::room_topology::KindCorridor: r.kind = "corridor"; break;
            case acc::room_topology::KindJunction: r.kind = "junction"; break;
            case acc::room_topology::KindDeadEnd:  r.kind = "dead end"; break;
            default:                               r.kind = "room"; break;
        }
        r.visited    = area == g_area && g_visited.count(r.id) != 0;
        r.here       = static_cast<int>(i) == g.si;
        r.reachable  = g.distOpen[i] < kFar;
        r.behindLock = !r.reachable && g.distAll[i] < kFar;
        r.walk       = r.reachable ? g.distOpen[i] : 0.0f;
        out.push_back(r);
    }
    for (const ClusterLink& l : g.links) links.emplace_back(l.a, l.b);
    return true;
}

}  // namespace acc::exit_memory

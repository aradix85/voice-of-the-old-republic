// Regions for the exit-memory modules: the region at a point, the region
// behind an exit, linked regions, discovered map notes per region, door
// numbering as the cycles say it, and what lies behind an exit.

#include "exit_memory_internal.h"

#include "cycle_state.h"
#include "engine_area.h"
#include "engine_compass.h"
#include "engine_player.h"     // position, yaw, party, player creature
#include "same_name_suffix.h"
#include "strings.h"

namespace acc::exit_memory {

namespace {

// Distances past the exit point, metres, at which the region behind it is
// probed (0 = the exit point itself, which for a direction exit can already
// lie in the next region). The first probe that lands in a region linked to
// the current one wins; failing that, the first different region.
constexpr float kProbeSteps[] = {0.0f, 1.5f, 3.0f, 5.0f, 8.0f};

}  // namespace

int RegionAt(void* area, const Vector& p) {
    std::string label;
    int sig = 0;
    int cid = kClusterIdNone;
    if (!acc::room_topology::LookupAt(area, p, label, sig, cid,
                                      /*allowDiagLog=*/false,
                                      /*requireWallReachable=*/true,
                                      /*zTrusted=*/false)) {
        return kClusterIdNone;
    }
    return cid;
}

// Regions sharing a nav edge with `self` (room_topology's region links).
std::set<int> LinkedRegions(void* area, int self) {
    std::set<int> out;
    std::vector<ClusterLink> links;
    acc::room_topology::GetClusterLinks(area, links);
    for (const ClusterLink& l : links) {
        if (l.a == self) out.insert(l.b);
        else if (l.b == self) out.insert(l.a);
    }
    return out;
}

int RegionBehind(void* area, int self, const std::set<int>& linked,
                 const Vector& from, const Vector& exitPos) {
    float dx = exitPos.x - from.x;
    float dy = exitPos.y - from.y;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.5f) return kClusterIdNone;
    dx /= len;
    dy /= len;
    int firstOther = kClusterIdNone;
    for (float d : kProbeSteps) {
        Vector p{exitPos.x + dx * d, exitPos.y + dy * d, exitPos.z};
        int cid = RegionAt(area, p);
        if (cid < 0 || cid == self) continue;
        if (linked.empty() || linked.count(cid)) return cid;
        if (firstOther < 0) firstOther = cid;
    }
    return firstOther;
}

// Discovered landmarks (map notes the player has met) with the region each
// one sits in. Only discovered ones, so the announce never spoils a place
// the player has not found.
std::vector<std::pair<int, std::string>> LandmarkRegions(void* area) {
    std::vector<std::pair<int, std::string>> out;
    acc::cycle::CategoryListing listing;
    if (!acc::cycle::BuildCategoryListing(CycleCategory::Landmark, listing)) {
        return out;
    }
    for (int i = 0; i < listing.count; ++i) {
        if (listing.isPin[i] || !listing.objs[i]) continue;
        char name[128] = {0};
        if (!acc::narration::GetSpokenName(listing.objs[i],
                                           CycleCategory::Landmark,
                                           name, sizeof(name)) ||
            !name[0]) {
            continue;
        }
        int cid = RegionAt(area, listing.positions[i]);
        if (cid >= 0) out.emplace_back(cid, name);
    }
    return out;
}

std::vector<DoorName> NumberedDoors() {
    std::vector<DoorName> out;
    acc::cycle::CategoryListing listing;
    if (!acc::cycle::BuildCategoryListing(CycleCategory::Door, listing)) {
        return out;
    }
    for (int i = 0; i < listing.count; ++i) {
        if (listing.isPin[i] || !listing.objs[i]) continue;
        char buf[160] = {0};
        if (!acc::engine::GetObjectBaseName(listing.objs[i], buf, sizeof(buf)) ||
            !buf[0]) {
            continue;
        }
        DoorName d;
        d.pos  = listing.positions[i];
        d.base = buf;
        acc::narration::AppendAreaPositionOrdinal(
            listing.objs[i], CycleCategory::Door, buf, sizeof(buf));
        d.numbered = buf;
        out.push_back(std::move(d));
    }
    return out;
}

void NumberDoorExit(ExitMark& e, const std::vector<DoorName>& doors) {
    for (const DoorName& d : doors) {
        if (DistXYSq(d.pos, e.pos) > 0.6f * 0.6f) continue;
        if (d.numbered.size() > d.base.size() &&
            e.text.compare(0, d.base.size(), d.base) == 0) {
            e.text = d.numbered + e.text.substr(d.base.size());
        }
        return;
    }
}

std::string Destination(void* area, int target,
                        const std::vector<std::pair<int, std::string>>& landmarks,
                        const std::string& exitText) {
    using acc::strings::Id;
    for (const auto& lm : landmarks) {
        if (lm.first != target) continue;
        // The label may already carry this landmark (door matched to it).
        if (!exitText.empty() && exitText.find(lm.second) != std::string::npos) {
            return std::string();
        }
        char buf[192];
        std::snprintf(buf, sizeof(buf), acc::strings::Get(Id::FmtExitAnnoTo),
                      lm.second.c_str());
        return buf;
    }
    ClusterInfo info;
    if (!acc::room_topology::GetClusterInfo(area, target, info)) {
        return std::string();
    }
    switch (info.kind) {
        case acc::room_topology::KindCorridor:
            return acc::strings::Get(Id::ExitAnnoToCorridor);
        case acc::room_topology::KindJunction:
            return acc::strings::Get(Id::ExitAnnoToJunction);
        case acc::room_topology::KindDeadEnd:
            return acc::strings::Get(Id::ExitAnnoToDeadEnd);
        case acc::room_topology::KindRoom:
        case acc::room_topology::KindPlatz:
        case acc::room_topology::KindOpenArea:
            return acc::strings::Get(Id::ExitAnnoToArea);
        default:
            return std::string();
    }
}

bool NearAny(const Vector& p, const std::vector<Vector>& points, float maxM) {
    for (const Vector& q : points) {
        if (DistXYSq(q, p) <= maxM * maxM) return true;
    }
    return false;
}

std::string Located(const std::string& subject, const Vector& pos) {
    Vector player;
    if (!acc::engine::GetPlayerPosition(player)) return subject;
    const float dx = pos.x - player.x;
    const float dy = pos.y - player.y;
    int metres = static_cast<int>(std::sqrt(dx * dx + dy * dy) + 0.5f);
    if (metres < 1) metres = 1;
    float yaw = 0.0f;
    char buf[640];
    if (acc::engine::GetPlayerYawDegrees(yaw)) {
        std::snprintf(buf, sizeof(buf),
                      acc::strings::Get(acc::strings::Id::FmtAnnounceWithClock),
                      subject.c_str(), acc::engine::ClockPosition(yaw, dx, dy),
                      metres);
    } else {
        std::snprintf(buf, sizeof(buf),
                      acc::strings::Get(acc::strings::Id::FmtAnnounceNoClock),
                      subject.c_str(), metres);
    }
    return buf;
}

}  // namespace acc::exit_memory

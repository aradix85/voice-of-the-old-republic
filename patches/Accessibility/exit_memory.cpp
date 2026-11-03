// Exit memory — see exit_memory.h.
#include "exit_memory.h"

#include <cmath>
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "cycle_state.h"
#include "engine_area.h"
#include "filter_objects.h"
#include "log.h"
#include "same_name_suffix.h"
#include "strings.h"

namespace acc::exit_memory {

namespace {

using acc::room_topology::ExitMark;
using acc::room_topology::kClusterIdNone;

std::map<std::string, std::set<int>> g_visitedByArea;
std::set<int>* g_visited = nullptr;
std::string    g_areaKey;
int            g_current  = kClusterIdNone;
int            g_previous = kClusterIdNone;

// Distances past the exit point, metres, at which the region behind it is
// probed. The first probe that lands in a different region wins.
constexpr float kProbeSteps[] = {1.5f, 3.0f, 5.0f, 8.0f};

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

int RegionBehind(void* area, const Vector& from, const Vector& exitPos) {
    float dx = exitPos.x - from.x;
    float dy = exitPos.y - from.y;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.5f) return kClusterIdNone;
    dx /= len;
    dy /= len;
    for (float d : kProbeSteps) {
        Vector p{exitPos.x + dx * d, exitPos.y + dy * d, exitPos.z};
        int cid = RegionAt(area, p);
        if (cid >= 0 && cid != g_current) return cid;
    }
    return kClusterIdNone;
}

// Discovered landmarks (map notes the player has met) with the region each
// one sits in. Only discovered ones, so the announce never spoils a place
// the player has not found.
std::vector<std::pair<int, std::string>> LandmarkRegions(void* area) {
    std::vector<std::pair<int, std::string>> out;
    acc::cycle::CategoryListing listing;
    if (!acc::cycle::BuildCategoryListing(acc::filter::CycleCategory::Landmark,
                                          listing)) {
        return out;
    }
    for (int i = 0; i < listing.count; ++i) {
        if (listing.isPin[i] || !listing.objs[i]) continue;
        char name[128] = {0};
        if (!acc::narration::GetSpokenName(listing.objs[i],
                                           acc::filter::CycleCategory::Landmark,
                                           name, sizeof(name)) ||
            !name[0]) {
            continue;
        }
        int cid = RegionAt(area, listing.positions[i]);
        if (cid >= 0) out.emplace_back(cid, name);
    }
    return out;
}

// Door exits carry the door's own position (room_topology::ExitPoint). Give
// such an exit the same number the Q/E and , / . cycles use ("Door 2"), so
// "Door East" in the region label and "Door 2" in the lists are visibly the
// same door: "Door East" becomes "Door 2 East".
struct DoorName {
    Vector      pos;
    std::string base;      // what the label's noun is ("Door", "Lift")
    std::string numbered;  // base + area ordinal ("Door 2"); == base if unique
};

std::vector<DoorName> NumberedDoors() {
    std::vector<DoorName> out;
    acc::cycle::CategoryListing listing;
    if (!acc::cycle::BuildCategoryListing(acc::filter::CycleCategory::Door,
                                          listing)) {
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
            listing.objs[i], acc::filter::CycleCategory::Door, buf, sizeof(buf));
        d.numbered = buf;
        out.push_back(std::move(d));
    }
    return out;
}

void NumberDoorExit(ExitMark& e, const std::vector<DoorName>& doors) {
    for (const DoorName& d : doors) {
        const float dx = d.pos.x - e.pos.x;
        const float dy = d.pos.y - e.pos.y;
        if (dx * dx + dy * dy > 0.6f * 0.6f) continue;
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
        if (exitText.find(lm.second) != std::string::npos) return std::string();
        char buf[192];
        std::snprintf(buf, sizeof(buf), acc::strings::Get(Id::FmtExitAnnoTo),
                      lm.second.c_str());
        return buf;
    }
    acc::room_topology::ClusterInfo info;
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

}  // namespace

void OnAreaEntered(void* area) {
    char key[64] = {0};
    if (!acc::engine::GetCurrentAreaResName(key, sizeof(key)) || !key[0]) {
        std::snprintf(key, sizeof(key), "%p", area);
    }
    g_areaKey  = key;
    g_visited  = &g_visitedByArea[g_areaKey];
    g_current  = kClusterIdNone;
    g_previous = kClusterIdNone;
    acclog::Write("ExitMemory", "area %s entered, %d region(s) remembered",
                  g_areaKey.c_str(), static_cast<int>(g_visited->size()));
}

void OnClusterCommitted(int from, int to) {
    if (to < 0 || to == g_current) return;
    g_previous = (from >= 0) ? from : g_current;
    g_current  = to;
    if (g_visited) g_visited->insert(to);
}

bool Annotate(void* area, const std::string& label,
              const std::vector<ExitMark>& exits,
              std::string& outLabel,
              std::vector<ExitMark>& outExits) {
    using acc::strings::Id;
    if (!area || g_current < 0 || !g_visited || exits.empty()) return false;

    // The label ends with the spoken exit entries joined by ", ".
    std::string joined;
    for (const ExitMark& e : exits) {
        if (e.text.empty()) continue;
        if (!joined.empty()) joined += ", ";
        joined += e.text;
    }
    if (joined.empty() || label.size() < joined.size() ||
        label.compare(label.size() - joined.size(), joined.size(), joined) != 0) {
        acclog::Write("ExitMemory", "label shape not recognised, plain: %s",
                      label.c_str());
        return false;
    }
    const std::string head = label.substr(0, label.size() - joined.size());

    acc::room_topology::ClusterInfo here;
    const bool haveCentre =
        acc::room_topology::GetClusterInfo(area, g_current, here);
    const auto landmarks = LandmarkRegions(area);
    const auto doors = NumberedDoors();

    outExits = exits;
    std::string list;
    bool added = false;
    for (ExitMark& e : outExits) {
        if (e.text.empty()) continue;
        const std::string before = e.text;
        NumberDoorExit(e, doors);
        if (e.text != before) added = true;
        const Vector from = haveCentre ? here.centre : e.pos;
        const int target = RegionBehind(area, from, e.pos);
        std::string extra;
        if (target >= 0) {
            if (target == g_previous) {
                extra = acc::strings::Get(Id::ExitAnnoBack);
            } else {
                std::string dest = Destination(area, target, landmarks, e.text);
                const char* status = g_visited->count(target)
                                         ? acc::strings::Get(Id::ExitAnnoVisited)
                                         : acc::strings::Get(Id::ExitAnnoNew);
                extra = dest.empty() ? std::string(status) : dest + ", " + status;
            }
        }
        if (!extra.empty()) {
            e.text += ", " + extra;
            added = true;
        }
        if (!list.empty()) list += "; ";
        list += e.text;
    }
    if (!added) return false;
    outLabel = head + list;
    acclog::Write("ExitMemory", "region %d (prev %d): %s", g_current,
                  g_previous, outLabel.c_str());
    return true;
}

}  // namespace acc::exit_memory

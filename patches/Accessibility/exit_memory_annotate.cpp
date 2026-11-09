// Region announcements and target lines: what lies behind each exit,
// visited or new, the way back, the "Here:" list, what is still behind a
// visited door, the current region on demand, and the way to a target.

#include "exit_memory_internal.h"

#include "discovery.h"         // loot heard in the overview joins the , / . cycle
#include "engine_area.h"
#include "engine_player.h"     // position, yaw, party, player creature
#include "engine_reads.h"      // ReadCreatureCurrentHitPoints
#include "log.h"
#include "same_name_suffix.h"
#include "strings.h"
#include "view_mode.h"         // routes are from the player, not the cursor

namespace acc::exit_memory {

namespace {

// What is still in a visited region behind a door: at most this many named.
constexpr int   kBehindMax       = 3;

// "Here:" names at most this many objects.
constexpr int   kHereMax     = 6;

struct RegionEntry {
    float       dist;
    std::string name;
};

// The people (not the party, not the dead), containers that still hold
// something and items on the ground standing in region `cluster`, nearest
// to the player first. `radiusM` <= 0: no distance limit. `record`: mark
// them heard for the , / . cycles.
std::vector<RegionEntry> RegionEntries(void* area, int cluster, float radiusM,
                                       bool record);

bool IsPartyMember(void* obj, const uint32_t* party, int count) {
    const uint32_t h = acc::engine::GetObjectHandle(obj);
    for (int i = 0; i < count; ++i) {
        if (party[i] == h) return true;
    }
    return false;
}

std::string HereList(void* area, int cluster) {
    std::vector<RegionEntry> found =
        RegionEntries(area, cluster, kHereRadiusM, /*record=*/true);
    if (found.empty()) return std::string();
    std::string list;
    int shown = 0;
    for (const RegionEntry& e : found) {
        if (shown == kHereMax) break;
        if (!list.empty()) list += "; ";
        list += e.name;
        ++shown;
    }
    const int rest = static_cast<int>(found.size()) - shown;
    if (rest > 0) {
        char more[64];
        std::snprintf(more, sizeof(more),
                      acc::strings::Get(acc::strings::Id::FmtRegionHereMore), rest);
        list += "; ";
        list += more;
    }
    char buf[1024];
    std::snprintf(buf, sizeof(buf),
                  acc::strings::Get(acc::strings::Id::FmtRegionHere), list.c_str());
    return buf;
}

std::vector<RegionEntry> RegionEntries(void* area, int cluster, float radiusM,
                                       bool record) {
    std::vector<RegionEntry> found;
    uint32_t party[12] = {0};
    const int partyCount = acc::engine::GetPartyMembers(party, 12);
    // Everything standing in the region, heard before or not: a sighted
    // player sees the whole room on walking in. (The , / . listing would
    // limit this to discovered objects whenever map-wide cycling is off,
    // and never holds ground items at all.)
    Vector player;
    if (!acc::engine::GetPlayerPosition(player)) return found;
    // The creature under control, by handle: pointer identity missed it and
    // "Here:" named the player herself ("Here: korasha; Remains 1").
    // With a companion as leader the player character is neither the
    // controlled creature nor in the follower table, so also skip it by its
    // chargen name.
    void* pc = acc::engine::GetPlayerServerCreature();
    const uint32_t pcHandle = pc ? acc::engine::GetObjectHandle(pc) : 0u;
    char pcName[128] = {0};
    acc::engine::GetPlayerCharacterName(pcName, sizeof(pcName));
    acc::engine::AreaObjectIterator it(area);
    while (void* obj = it.Next()) {
        if (obj == pc) continue;
        if (pcHandle != 0 && acc::engine::GetObjectHandle(obj) == pcHandle) {
            continue;
        }
        CycleCategory cat;
        if (acc::filter::ObjectMatches(obj, CycleCategory::Npc)) {
            cat = CycleCategory::Npc;
        } else if (acc::filter::ObjectMatches(obj, CycleCategory::Container)) {
            cat = CycleCategory::Container;
        } else if (acc::filter::ObjectMatches(obj, CycleCategory::Item)) {
            cat = CycleCategory::Item;
        } else {
            continue;
        }
        Vector pos;
        if (!acc::engine::GetObjectPosition(obj, pos)) continue;
        const float dist = DistXY(pos, player);
        if (radiusM > 0.0f && dist > radiusM) continue;
        if (cat == CycleCategory::Npc) {
            if (IsPartyMember(obj, party, partyCount)) continue;
            const int hp = acc::engine::ReadCreatureCurrentHitPoints(obj);
            if (hp == 0 || hp < -1) continue;  // dead
        }
        if (cat == CycleCategory::Container &&
            acc::engine::IsEmptyContainer(obj)) {
            continue;  // looted: nothing to come back for
        }
        if (RegionAt(area, pos) != cluster) continue;
        char name[160] = {0};
        if (!acc::narration::GetSpokenName(obj, cat, name, sizeof(name)) ||
            !name[0]) {
            continue;
        }
        if (cat == CycleCategory::Npc && pcName[0] &&
            ToLowerAscii(name) == ToLowerAscii(pcName)) {
            continue;
        }
        found.push_back(RegionEntry{dist, name});
        // Heard now: the , / . cycles may list it from here on.
        if (record) acc::discovery::Record(obj);
    }
    std::stable_sort(found.begin(), found.end(),
                     [](const RegionEntry& a, const RegionEntry& b) {
                         return a.dist < b.dist;
                     });
    return found;
}

// The player's region label, annotated like the walk-in announce when the
// committed region is the one underfoot.
std::string RegionLabelAt(void* area, const Vector& player) {
    std::string label;
    int sig = 0;
    int cid = kClusterIdNone;
    std::vector<ExitMark> exits;
    if (!acc::room_topology::LookupAt(area, player, label, sig, cid,
                                      /*allowDiagLog=*/false,
                                      /*requireWallReachable=*/true,
                                      /*zTrusted=*/true, &exits) ||
        label.empty()) {
        return std::string();
    }
    // In the doorway sliver of an area door: read the region in front of it.
    Vector ownerCentre{};
    if (TransitionSliverOwner(area, cid, &ownerCentre) != cid) {
        std::string ownerLabel;
        std::vector<ExitMark> ownerExits;
        int ownerCid = kClusterIdNone;
        if (acc::room_topology::LookupAt(area, ownerCentre, ownerLabel, sig,
                                         ownerCid, /*allowDiagLog=*/false,
                                         /*requireWallReachable=*/true,
                                         /*zTrusted=*/false, &ownerExits) &&
            !ownerLabel.empty()) {
            label = ownerLabel;
            exits = ownerExits;
            cid   = ownerCid;
        }
    }
    if (area == g_area && cid >= 0 && cid == g_current) {
        std::string annotated;
        std::vector<ExitMark> unused;
        if (Annotate(area, label, exits, annotated, unused)) return annotated;
    }
    return label;
}

}  // namespace

// What is still in a visited region behind a door ("Matrik; Footlocker"),
// or "nothing left". Empty for a region not visited: the player has not
// seen it, and naming it would spoil it.
std::string BehindDoorText(void* area, int farId) {
    if (area != g_area || farId < 0 || !g_visited.count(farId)) {
        return std::string();
    }
    std::vector<RegionEntry> found =
        RegionEntries(area, farId, 0.0f, /*record=*/false);
    if (found.empty()) return acc::strings::Get(acc::strings::Id::ExitAnnoNothingLeft);
    std::string list;
    int shown = 0;
    for (const RegionEntry& e : found) {
        if (shown == kBehindMax) break;
        if (!list.empty()) list += "; ";
        list += e.name;
        ++shown;
    }
    const int rest = static_cast<int>(found.size()) - shown;
    if (rest > 0) {
        char more[64];
        std::snprintf(more, sizeof(more),
                      acc::strings::Get(acc::strings::Id::FmtRegionHereMore), rest);
        list += "; ";
        list += more;
    }
    return list;
}

bool Annotate(void* area, const std::string& label,
              const std::vector<ExitMark>& exits,
              std::string& outLabel,
              std::vector<ExitMark>& outExits) {
    using acc::strings::Id;
    if (!area || g_current < 0) return false;

    // The label ends with the spoken exit entries joined by ", ".
    std::string head;
    bool shapeOk = false;
    {
        std::string joined;
        for (const ExitMark& e : exits) {
            if (e.text.empty()) continue;
            if (!joined.empty()) joined += ", ";
            joined += e.text;
        }
        if (!joined.empty()) {
            if (label.size() >= joined.size() &&
                label.compare(label.size() - joined.size(), joined.size(),
                              joined) == 0) {
                head = label.substr(0, label.size() - joined.size());
                shapeOk = true;
            } else {
                acclog::Write("ExitMemory",
                              "label shape not recognised, exits plain: %s",
                              label.c_str());
            }
        }
    }

    std::vector<ExitMark> annotated = exits;
    std::string list;
    bool exitsAdded = false;
    if (shapeOk) {
        ClusterInfo here;
        const bool haveCentre =
            acc::room_topology::GetClusterInfo(area, g_current, here);
        const auto landmarks = LandmarkRegions(area);
        const auto doors = NumberedDoors();
        const std::set<int> linked = LinkedRegions(area, g_current);
        for (ExitMark& e : annotated) {
            if (e.text.empty()) continue;
            const std::string before = e.text;
            NumberDoorExit(e, doors);
            if (e.text != before) exitsAdded = true;
            const Vector from = haveCentre ? here.centre : e.pos;
            const int target = RegionBehind(area, g_current, linked, from, e.pos);
            std::string extra;
            if (target >= 0) {
                if (target == g_previous) {
                    extra = acc::strings::Get(Id::ExitAnnoBack);
                } else {
                    std::string dest = Destination(area, target, landmarks, e.text);
                    const char* status = g_visited.count(target)
                                             ? acc::strings::Get(Id::ExitAnnoVisited)
                                             : acc::strings::Get(Id::ExitAnnoNew);
                    extra = dest.empty() ? std::string(status) : dest + ", " + status;
                }
            }
            // A door into another area: the notes of the area it led to.
            // Any other door: what is still behind it, once visited.
            Vector doorPos = e.pos;
            if (TransitionDoorNear(area, e.pos, kDoorMatchM, std::string(),
                                   &doorPos)) {
                const std::string mem = TransitionDoorMemory(doorPos);
                if (!mem.empty()) extra = extra.empty() ? mem : extra + ", " + mem;
            } else if (target >= 0 && target != g_previous &&
                       DoorNear(area, e.pos)) {
                const std::string behind = BehindDoorText(area, target);
                if (!behind.empty()) {
                    extra = extra.empty() ? behind : extra + ", " + behind;
                }
            }
            if (!extra.empty()) {
                e.text += ", " + extra;
                exitsAdded = true;
            }
            if (!list.empty()) list += "; ";
            list += e.text;
        }
    }

    const std::string hereList = HereList(area, g_current);
    if (!exitsAdded && hereList.empty()) return false;

    std::string text = exitsAdded ? head + list : label;
    outExits = exitsAdded ? annotated : exits;
    if (!hereList.empty()) {
        if (!text.empty() && text.back() != '.') text += '.';
        text += ' ';
        text += hereList;
    }
    outLabel = text;
    acclog::Write("ExitMemory", "region %d (prev %d): %s", g_current,
                  g_previous, outLabel.c_str());
    return true;
}

std::string CurrentRegionText() {
    Vector player;
    if (!acc::engine::GetPlayerPosition(player)) return std::string();
    void* area = acc::engine::GetCurrentArea();
    if (!area) return std::string();
    return RegionLabelAt(area, player);
}

std::string TargetWayInfo(void* obj, const Vector& pos) {
    if (!obj || acc::view_mode::IsActive()) return std::string();
    Vector player;
    if (!acc::engine::GetPlayerPosition(player)) return std::string();
    Graph g;
    if (!BuildGraph(acc::engine::GetCurrentArea(), player, g) || g.si < 0) {
        return std::string();
    }
    if (acc::filter::ObjectMatches(obj, CycleCategory::Door)) {
        if (acc::engine::IsDoorCosmetic(obj)) return std::string();
        // A door into another area already says where it goes (its
        // TransitionDestination is in the label). The sliver of this area's
        // walkmesh behind it is not a place: "leads South, to a dead end,
        // new" on the Upper City Cantina entrance only sounded like a place
        // still to explore. Give just the way to the door (to its near
        // side: the door's own position can fall in that sliver).
        int nearIdx = -1;
        std::string out = DoorBeyond(g, pos, &nearIdx);
        // Instead: what the area behind it holds, once the player has been
        // through ("Matrik's Apartment"), so two areas of the same name
        // differ.
        if (acc::engine::DoorHasTransitionDest(obj)) out = TransitionDoorMemory(pos);
        if (nearIdx < 0) return out.empty() ? RouteText(RouteTo(g, pos)) : out;
        if (nearIdx != g.si) {
            const std::string route = RouteText(RouteToIndex(g, nearIdx, pos));
            if (!route.empty()) {
                if (!out.empty()) out += ", ";
                out += route;
            }
        }
        return out;
    }
    return RouteText(RouteTo(g, pos));
}

}  // namespace acc::exit_memory

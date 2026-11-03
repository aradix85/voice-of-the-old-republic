// Exit memory — see exit_memory.h.
#include "exit_memory.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "cycle_input.h"       // WalkToNarratedTarget
#include "cycle_state.h"
#include "engine_area.h"
#include "engine_compass.h"
#include "engine_panels.h"     // IsForegroundUiBlocking
#include "engine_player.h"     // position, yaw, party, player creature
#include "engine_reads.h"      // ReadCreatureCurrentHitPoints
#include "engine_scriptvar.h"  // visited regions in the save
#include "filter_objects.h"
#include "guidance_approach.h"
#include "guidance_autowalk.h"
#include "hotkeys.h"
#include "narrated_target.h"
#include "log.h"
#include "prism.h"
#include "same_name_suffix.h"
#include "strings.h"

namespace acc::exit_memory {

namespace {

using acc::filter::CycleCategory;
using acc::room_topology::ClusterInfo;
using acc::room_topology::ClusterLink;
using acc::room_topology::ClusterSummary;
using acc::room_topology::ExitMark;
using acc::room_topology::kClusterIdNone;

void*         g_area = nullptr;
std::string   g_areaKey;   // lower-case module resref
std::string   g_varName;   // ACC_VISIT_<areaKey>, empty = not persisted
std::set<int> g_visited;
int           g_current  = kClusterIdNone;
int           g_previous = kClusterIdNone;

// Save-var reconciliation, like discovery: read only once the player
// creature has been the same pointer for a settle window (a read on the
// save-load tick races the engine restoring the var table).
bool  g_saveLoaded     = false;
void* g_settleCreature = nullptr;
int   g_settleTicks    = 0;
constexpr int    kSettleTicks     = 60;
constexpr size_t kMaxSavedRegions = 300;
// A saved region centre matches a live region whose centre is this close.
constexpr float  kSavedMatchM     = 2.5f;

// Distances past the exit point, metres, at which the region behind it is
// probed (0 = the exit point itself, which for a direction exit can already
// lie in the next region). The first probe that lands in a region linked to
// the current one wins; failing that, the first different region.
constexpr float kProbeSteps[] = {0.0f, 1.5f, 3.0f, 5.0f, 8.0f};

// "Here:" — objects farther than this from the player are not considered
// (a cheap pre-filter before the region test), and at most this many are
// named.
constexpr float kHereRadiusM = 30.0f;
constexpr int   kHereMax     = 6;

// Overview: loot spots named (the rest are counted).
constexpr int   kLootSpoken  = 3;
// A link through a door counts as locked when a locked, closed door
// stands this close to the door the link was recorded with.
constexpr float kDoorMatchM  = 1.5f;

constexpr float kFar = 1e30f;

std::string ToLowerAscii(const char* s) {
    std::string out = s ? s : "";
    for (char& c : out) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return out;
}

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

// ---------------------------------------------------------------------------
// Visited regions in the save

void Persist() {
    if (!g_saveLoaded || g_varName.empty() || !g_area) return;
    std::string val;
    size_t n = 0;
    for (int id : g_visited) {
        ClusterInfo info;
        if (!acc::room_topology::GetClusterInfo(g_area, id, info)) continue;
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%ld,%ld", std::lround(info.centre.x),
                      std::lround(info.centre.y));
        if (!val.empty()) val.push_back(';');
        val += buf;
        if (++n >= kMaxSavedRegions) break;
    }
    acc::engine::SetPlayerVarString(g_varName.c_str(), val.c_str());
}

void RecordVisit(int id) {
    if (id < 0) return;
    if (g_visited.insert(id).second) Persist();
}

void LoadVisited() {
    std::vector<ClusterSummary> clusters;
    acc::room_topology::ListClusters(g_area, clusters);
    static char buf[8192];
    std::set<int> saved;
    int entries = 0;
    if (acc::engine::GetPlayerVarString(g_varName.c_str(), buf, sizeof(buf))) {
        const char* p = buf;
        while (*p) {
            char* end = nullptr;
            const long x = std::strtol(p, &end, 10);
            if (end == p || *end != ',') break;
            p = end + 1;
            const long y = std::strtol(p, &end, 10);
            if (end == p) break;
            p = end;
            ++entries;
            int best = kClusterIdNone;
            float bestSq = kSavedMatchM * kSavedMatchM;
            for (const ClusterSummary& c : clusters) {
                const float dx = c.centre.x - static_cast<float>(x);
                const float dy = c.centre.y - static_cast<float>(y);
                const float d2 = dx * dx + dy * dy;
                if (d2 <= bestSq) {
                    bestSq = d2;
                    best = c.id;
                }
            }
            if (best >= 0) saved.insert(best);
            if (*p == ';') ++p;
            else if (*p) break;
        }
    }
    bool newSinceEntry = false;
    for (int id : g_visited) {
        if (!saved.count(id)) {
            newSinceEntry = true;
            break;
        }
    }
    g_visited.insert(saved.begin(), saved.end());
    g_saveLoaded = true;
    if (newSinceEntry) Persist();
    acclog::Write("ExitMemory",
                  "loaded %s: %d saved entr%s -> %d region(s), %d visited now",
                  g_varName.c_str(), entries, entries == 1 ? "y" : "ies",
                  static_cast<int>(saved.size()),
                  static_cast<int>(g_visited.size()));
}

// ---------------------------------------------------------------------------
// "Here:" list

bool IsPartyMember(void* obj, const uint32_t* party, int count) {
    const uint32_t h = acc::engine::GetObjectHandle(obj);
    for (int i = 0; i < count; ++i) {
        if (party[i] == h) return true;
    }
    return false;
}

std::string HereList(void* area, int cluster) {
    struct Entry {
        float       dist;
        std::string name;
    };
    std::vector<Entry> found;
    uint32_t party[12] = {0};
    const int partyCount = acc::engine::GetPartyMembers(party, 12);
    const CycleCategory cats[] = {CycleCategory::Npc, CycleCategory::Container,
                                  CycleCategory::Item};
    for (CycleCategory cat : cats) {
        acc::cycle::CategoryListing listing;
        if (!acc::cycle::BuildCategoryListing(cat, listing)) continue;
        for (int i = 0; i < listing.count; ++i) {
            void* obj = listing.objs[i];
            if (listing.isPin[i] || !obj) continue;
            if (listing.distances[i] > kHereRadiusM) continue;
            if (cat == CycleCategory::Npc) {
                if (IsPartyMember(obj, party, partyCount)) continue;
                const int hp = acc::engine::ReadCreatureCurrentHitPoints(obj);
                if (hp == 0 || hp < -1) continue;  // dead
            }
            if (cat == CycleCategory::Container &&
                acc::engine::IsEmptyContainer(obj)) {
                continue;  // looted: nothing to come back for
            }
            if (RegionAt(area, listing.positions[i]) != cluster) continue;
            char name[160] = {0};
            if (!acc::narration::GetSpokenName(obj, cat, name, sizeof(name)) ||
                !name[0]) {
                continue;
            }
            found.push_back(Entry{listing.distances[i], name});
        }
    }
    if (found.empty()) return std::string();
    std::stable_sort(found.begin(), found.end(),
                     [](const Entry& a, const Entry& b) { return a.dist < b.dist; });
    std::string list;
    int shown = 0;
    for (const Entry& e : found) {
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

// ---------------------------------------------------------------------------
// Overview helpers

using Adjacency = std::vector<std::vector<std::pair<int, float>>>;

void ShortestPaths(const Adjacency& adj, int start, std::vector<float>& dist,
                   std::vector<int>& firstHop) {
    const int n = static_cast<int>(adj.size());
    dist.assign(n, kFar);
    firstHop.assign(n, -1);
    if (start < 0 || start >= n) return;
    std::vector<bool> done(n, false);
    dist[start] = 0.0f;
    for (;;) {
        int u = -1;
        float best = kFar;
        for (int i = 0; i < n; ++i) {
            if (!done[i] && dist[i] < best) {
                best = dist[i];
                u = i;
            }
        }
        if (u < 0) break;
        done[u] = true;
        for (const auto& edge : adj[u]) {
            const int v = edge.first;
            const float nd = dist[u] + edge.second;
            if (nd < dist[v]) {
                dist[v] = nd;
                firstHop[v] = (u == start) ? v : firstHop[u];
            }
        }
    }
}

std::vector<Vector> LockedDoorPositions(void* area) {
    std::vector<Vector> out;
    acc::engine::AreaObjectIterator it(area);
    while (void* obj = it.Next()) {
        if (acc::engine::GetObjectKind(obj) !=
            static_cast<int>(acc::engine::GameObjectKind::Door)) {
            continue;
        }
        if (!acc::engine::IsDoorLocked(obj) || acc::engine::IsDoorOpen(obj)) {
            continue;
        }
        Vector p;
        if (acc::engine::GetObjectPosition(obj, p)) out.push_back(p);
    }
    return out;
}

// A closed door standing within kDoorMatchM of `p`, or null.
void* ClosedDoorNear(void* area, const Vector& p) {
    acc::engine::AreaObjectIterator it(area);
    while (void* obj = it.Next()) {
        if (acc::engine::GetObjectKind(obj) !=
            static_cast<int>(acc::engine::GameObjectKind::Door)) {
            continue;
        }
        if (acc::engine::IsDoorOpen(obj)) continue;
        Vector q;
        if (!acc::engine::GetObjectPosition(obj, q)) continue;
        const float dx = q.x - p.x;
        const float dy = q.y - p.y;
        if (dx * dx + dy * dy <= kDoorMatchM * kDoorMatchM) return obj;
    }
    return nullptr;
}

bool NearAny(const Vector& p, const std::vector<Vector>& points, float maxM) {
    for (const Vector& q : points) {
        const float dx = q.x - p.x;
        const float dy = q.y - p.y;
        if (dx * dx + dy * dy <= maxM * maxM) return true;
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

}  // namespace

void OnAreaEntered(void* area) {
    char key[64] = {0};
    if (!acc::engine::GetCurrentAreaResName(key, sizeof(key))) key[0] = '\0';
    g_area     = area;
    g_areaKey  = ToLowerAscii(key);
    g_varName  = g_areaKey.empty() ? std::string()
                                   : std::string("ACC_VISIT_") + g_areaKey;
    g_visited.clear();
    g_current  = kClusterIdNone;
    g_previous = kClusterIdNone;
    g_saveLoaded     = false;
    g_settleCreature = nullptr;
    g_settleTicks    = 0;
    acclog::Write("ExitMemory", "area %s entered (save var %s, load deferred)",
                  g_areaKey.c_str(), g_varName.c_str());
}

void OnClusterCommitted(int from, int to) {
    if (to < 0 || to == g_current) return;
    g_previous = (from >= 0) ? from : g_current;
    g_current  = to;
    RecordVisit(to);
}

void OnClusterSeen(int id) {
    RecordVisit(id);
}

void Tick() {
    if (g_saveLoaded || !g_area) return;
    if (g_varName.empty()) {
        g_saveLoaded = true;  // no stable key: session-only memory
        return;
    }
    if (acc::engine::GetCurrentArea() != g_area) return;
    if (!acc::room_topology::HasGraphForArea(g_area)) return;
    void* cre = acc::engine::GetPlayerServerCreature();
    if (!cre) {
        g_settleCreature = nullptr;
        g_settleTicks = 0;
        return;
    }
    if (cre != g_settleCreature) {
        g_settleCreature = cre;
        g_settleTicks = 1;
        return;
    }
    if (++g_settleTicks < kSettleTicks) return;
    LoadVisited();
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

bool CollectOverview(Overview& o) {
    o = Overview{};
    Vector player;
    if (!acc::engine::GetPlayerPosition(player)) return false;
    void* area = acc::engine::GetCurrentArea();
    if (!area) return false;

    std::vector<ClusterSummary> nodes;
    if (acc::room_topology::ListClusters(area, nodes) && !nodes.empty()) {
        o.haveGraph = true;

        // The player's region and its exits, straight from the graph.
        std::string label;
        int sig = 0;
        int underPlayer = kClusterIdNone;
        std::vector<ExitMark> exits;
        acc::room_topology::LookupAt(area, player, label, sig, underPlayer,
                                     /*allowDiagLog=*/false,
                                     /*requireWallReachable=*/true,
                                     /*zTrusted=*/true, &exits);
        const int start = g_current >= 0 ? g_current : underPlayer;

        auto indexOf = [&nodes](int id) {
            for (size_t i = 0; i < nodes.size(); ++i) {
                if (nodes[i].id == id) return static_cast<int>(i);
            }
            return -1;
        };

        std::vector<ClusterLink> links;
        acc::room_topology::GetClusterLinks(area, links);
        const std::vector<Vector> lockedDoors = LockedDoorPositions(area);
        const std::set<int> linked = LinkedRegions(area, start);
        Adjacency open(nodes.size()), all(nodes.size());
        for (const ClusterLink& l : links) {
            const int a = indexOf(l.a);
            const int b = indexOf(l.b);
            if (a < 0 || b < 0) continue;
            const float dx = nodes[a].centre.x - nodes[b].centre.x;
            const float dy = nodes[a].centre.y - nodes[b].centre.y;
            const float w = (std::max)(0.5f, std::sqrt(dx * dx + dy * dy));
            all[a].emplace_back(b, w);
            all[b].emplace_back(a, w);
            if (l.viaDoor && NearAny(l.doorPos, lockedDoors, kDoorMatchM)) continue;
            open[a].emplace_back(b, w);
            open[b].emplace_back(a, w);
        }

        const int si = indexOf(start);
        std::vector<float> distOpen, distAll;
        std::vector<int> hopOpen, hopAll;
        ShortestPaths(open, si, distOpen, hopOpen);
        ShortestPaths(all, si, distAll, hopAll);

        int target = -1;
        for (size_t i = 0; i < nodes.size(); ++i) {
            const bool visited = g_visited.count(nodes[i].id) != 0;
            const bool reachable = distAll[i] < kFar;
            if (visited || reachable) ++o.total;
            if (visited) ++o.explored;
            if (visited || !reachable || static_cast<int>(i) == si) continue;
            if (distOpen[i] >= kFar) {
                ++o.lockedAway;
            } else if (target < 0 || distOpen[i] < distOpen[target]) {
                target = static_cast<int>(i);
            }
        }

        if (target >= 0 && hopOpen[target] >= 0) {
            o.haveTarget = true;
            const auto landmarks = LandmarkRegions(area);
            o.targetWhat = Destination(area, nodes[target].id, landmarks,
                                       std::string());
            const int hop = hopOpen[target];
            const int hopId = nodes[hop].id;
            o.stepPos = nodes[hop].centre;
            // The closed door on the link into the first region, if any:
            // walking there means opening it.
            for (const ClusterLink& l : links) {
                const bool thisLink = (l.a == start && l.b == hopId) ||
                                      (l.b == start && l.a == hopId);
                if (!thisLink || !l.viaDoor) continue;
                o.stepDoor = ClosedDoorNear(area, l.doorPos);
                break;
            }
            // The exit of the current region that leads to the first region
            // on the way (the label's exits belong to the region under the
            // player).
            if (underPlayer == start && !exits.empty()) {
                ClusterInfo here;
                const bool haveCentre =
                    acc::room_topology::GetClusterInfo(area, start, here);
                const auto doors = NumberedDoors();
                for (ExitMark e : exits) {
                    if (e.text.empty()) continue;
                    const Vector from = haveCentre ? here.centre : e.pos;
                    if (RegionBehind(area, start, linked, from, e.pos) != hopId) {
                        continue;
                    }
                    NumberDoorExit(e, doors);
                    o.via = e.text;
                    o.targetPos = e.pos;
                    break;
                }
            }
            if (o.via.empty()) o.targetPos = nodes[hop].centre;
        }
    }

    // Loot left: containers holding items, and items on the ground.
    const CycleCategory cats[] = {CycleCategory::Container, CycleCategory::Item};
    for (CycleCategory cat : cats) {
        acc::cycle::CategoryListing listing;
        if (!acc::cycle::BuildCategoryListing(cat, listing)) continue;
        for (int i = 0; i < listing.count; ++i) {
            void* obj = listing.objs[i];
            if (listing.isPin[i] || !obj) continue;
            if (cat == CycleCategory::Container &&
                acc::engine::GetContainerItemCount(obj) <= 0) {
                continue;
            }
            char name[160] = {0};
            if (!acc::narration::GetSpokenName(obj, cat, name, sizeof(name)) ||
                !name[0]) {
                continue;
            }
            LootSpot s;
            s.obj      = obj;
            s.handle   = acc::engine::GetObjectHandle(obj);
            s.name     = name;
            s.pos      = listing.positions[i];
            s.distance = listing.distances[i];
            o.loot.push_back(std::move(s));
        }
    }
    std::stable_sort(o.loot.begin(), o.loot.end(),
                     [](const LootSpot& a, const LootSpot& b) {
                         return a.distance < b.distance;
                     });
    return true;
}

std::string OverviewText(const Overview& o) {
    using acc::strings::Id;
    std::vector<std::string> parts;
    char buf[768];
    if (!o.haveGraph) {
        parts.push_back(acc::strings::Get(Id::OverviewNoMap));
    } else {
        std::snprintf(buf, sizeof(buf), acc::strings::Get(Id::FmtOverviewExplored),
                      o.explored, o.total);
        parts.push_back(buf);
        if (o.haveTarget) {
            std::string subject = o.via;
            if (!o.targetWhat.empty()) {
                if (!subject.empty()) subject += ", ";
                subject += o.targetWhat;
            }
            if (subject.empty()) subject = acc::strings::Get(Id::ExitAnnoNew);
            std::snprintf(buf, sizeof(buf), acc::strings::Get(Id::FmtOverviewNearest),
                          Located(subject, o.targetPos).c_str());
            parts.push_back(buf);
        } else if (o.lockedAway == 0) {
            parts.push_back(acc::strings::Get(Id::OverviewAllExplored));
        }
        if (o.lockedAway > 0) {
            std::snprintf(buf, sizeof(buf),
                          acc::strings::Get(Id::FmtOverviewLockedAway), o.lockedAway);
            parts.push_back(buf);
        }
    }
    if (o.loot.empty()) {
        parts.push_back(acc::strings::Get(Id::OverviewNothingToSearch));
    } else {
        std::snprintf(buf, sizeof(buf), acc::strings::Get(Id::FmtOverviewToSearch),
                      static_cast<int>(o.loot.size()));
        parts.push_back(buf);
        std::string spots;
        int n = 0;
        for (const LootSpot& s : o.loot) {
            if (n == kLootSpoken) break;
            if (!spots.empty()) spots += "; ";
            spots += Located(s.name, s.pos);
            ++n;
        }
        parts.push_back(spots);
    }
    std::string text;
    for (const std::string& p : parts) {
        if (p.empty()) continue;
        if (!text.empty()) text += ". ";
        text += p;
    }
    if (!text.empty() && text.back() != '.') text += '.';
    return text;
}

void PollOverviewHotkey() {
    if (!acc::hotkeys::Pressed(acc::hotkeys::Action::ExploreOverview)) return;
    Vector unused;
    if (!acc::engine::GetPlayerPosition(unused)) return;
    if (acc::engine::IsForegroundUiBlocking()) return;
    Overview o;
    if (!CollectOverview(o)) return;
    const std::string text = OverviewText(o);
    prism::Speak(text.c_str(), /*interrupt=*/true);
    acclog::Write("ExitMemory", "overview: %s", text.c_str());
}

namespace {

void WalkTowardUnexplored() {
    using acc::strings::Id;
    Overview o;
    if (!CollectOverview(o)) return;
    if (!o.haveTarget) {
        // Nothing reachable left: say why (all explored / behind locks).
        const std::string text = OverviewText(o);
        prism::Speak(text.c_str(), /*interrupt=*/true);
        return;
    }
    std::string subject = o.via;
    if (!o.targetWhat.empty()) {
        if (!subject.empty()) subject += ", ";
        subject += o.targetWhat;
    }
    if (subject.empty()) subject = acc::strings::Get(Id::ExitAnnoNew);

    if (o.stepDoor) {
        // A closed door on the way: walk to it and open it (Shift+- on it).
        acc::narrated_target::Stamp(o.stepDoor,
                                    acc::engine::GetObjectHandle(o.stepDoor));
        acc::cycle_input::WalkToNarratedTarget();
        acclog::Write("ExitMemory", "walk unexplored: via door (%s)",
                      subject.c_str());
        return;
    }
    if (!acc::guidance::WalkTo(o.stepPos)) {
        char fail[256];
        std::snprintf(fail, sizeof(fail), acc::strings::Get(Id::FmtGuidingFailed),
                      subject.c_str());
        prism::Speak(fail, /*interrupt=*/true);
        return;
    }
    char buf[640];
    std::snprintf(buf, sizeof(buf), acc::strings::Get(Id::FmtWalkingTo),
                  Located(subject, o.stepPos).c_str());
    prism::Speak(buf, /*interrupt=*/true);
    acc::guidance::ApproachArm arm;
    arm.owner = acc::guidance::ApproachOwner::Cycle;
    std::snprintf(arm.name, sizeof(arm.name), "%s", subject.c_str());
    arm.targetPos    = o.stepPos;
    arm.speakBlocked = true;
    acc::guidance::ArmApproach(arm);
    acclog::Write("ExitMemory", "walk unexplored: %s -> (%.1f, %.1f)", buf,
                  o.stepPos.x, o.stepPos.y);
}

void WalkToNearestLoot() {
    Overview o;
    if (!CollectOverview(o)) return;
    if (o.loot.empty()) {
        prism::Speak(acc::strings::Get(acc::strings::Id::OverviewNothingToSearch),
                     /*interrupt=*/true);
        return;
    }
    const LootSpot& s = o.loot.front();
    if (!s.obj || !s.handle) return;
    acc::narrated_target::Stamp(s.obj, s.handle);
    acc::cycle_input::WalkToNarratedTarget();
    acclog::Write("ExitMemory", "walk loot: %s", s.name.c_str());
}

}  // namespace

void PollWalkHotkeys() {
    const bool unexplored =
        acc::hotkeys::Pressed(acc::hotkeys::Action::ExploreWalkUnexplored);
    const bool loot = acc::hotkeys::Pressed(acc::hotkeys::Action::ExploreWalkLoot);
    if (!unexplored && !loot) return;
    Vector unused;
    if (!acc::engine::GetPlayerPosition(unused)) return;
    if (acc::engine::IsForegroundUiBlocking()) return;
    if (unexplored) {
        WalkTowardUnexplored();
    } else {
        WalkToNearestLoot();
    }
}

}  // namespace acc::exit_memory

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

#include <windows.h>           // GetTickCount (loot reminder timing)

#include "cycle_input.h"       // WalkToNarratedTarget
#include "cycle_state.h"
#include "discovery.h"         // loot heard in the overview joins the , / . cycle
#include "endar_softlock.h"    // Trask may not try his own door
#include "engine_area.h"
#include "engine_compass.h"
#include "engine_panels.h"     // IsForegroundUiBlocking
#include "engine_player.h"     // position, yaw, party, player creature
#include "engine_reads.h"      // ReadCreatureCurrentHitPoints
#include "engine_scriptvar.h"  // visited regions in the save
#include "engine_subscreen.h"  // WorldIsPaused (explore walk waits)
#include "filter_objects.h"
#include "guidance_approach.h"
#include "guidance_autowalk.h"
#include "hotkeys.h"
#include "interact_dispatch.h" // Ctrl+Shift+N tries a locked door like Enter
#include "narrated_target.h"
#include "log.h"
#include "menus_modsettings.h"   // ExtendedCycling: list loot map-wide
#include "prism.h"
#include "same_name_suffix.h"
#include "strings.h"
#include "view_mode.h"         // routes are from the player, not the cursor

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
constexpr int   kLootSpoken  = 5;
// Walking distances are spoken rounded to this many metres.
constexpr float kWalkRoundM  = 5.0f;

// Loot reminder (RemindLootOnLeaving): spoken this long after the walk line,
// queued behind it; not again for the same door within the repeat window;
// at most this many things named.
constexpr DWORD kReminderDelayMs  = 1200;
constexpr DWORD kReminderRepeatMs = 20000;
constexpr int   kReminderNamed    = 3;

// "No way there now" after an Enter on something only behind locked doors:
// queued behind the walk line like the reminder.
constexpr DWORD kNoWayDelayMs = 700;
bool        g_noWayPending    = false;
DWORD       g_noWayAtMs       = 0;

bool        g_reminderPending = false;
DWORD       g_reminderAtMs    = 0;
std::string g_reminderText;
uint32_t    g_reminderDoor    = 0;
DWORD       g_reminderDoorMs  = 0;
// A link through a door counts as locked when a locked, closed door
// stands this close to the door the link was recorded with.
constexpr float kDoorMatchM  = 1.5f;

// Ctrl+Shift+N walks the whole way to the nearest unexplored region, one
// region hop after another: each hop is a short walk to the next region's
// centre, which the engine reliably completes (a single long walk across
// several regions and doors does not). The goal region is fixed when the key
// is pressed, so the walk does not turn round when another unexplored region
// becomes nearer on the way. Region ids are only kept for this one walk;
// nothing is saved by id (ids are not stable across loads).
struct ExploreChain {
    bool        active  = false;
    int         goal    = -1;
    uint32_t    serial  = 0;      // approach serial of the current hop
    int         hops    = 0;
    DWORD       endedAt = 0;      // when the current hop's approach ended
    Vector      stepFrom = {0.0f, 0.0f, 0.0f};  // where the current hop began
    void*       door    = nullptr;  // door the current hop opens, if any
    int         doorTries = 0;      // hops in a row at that same door
    std::string name;             // "to a corridor": for way-blocked lines
};
ExploreChain g_chain;
constexpr int   kChainMaxHops  = 40;
constexpr DWORD kChainSettleMs = 500;   // after a hop: door swing, region commit
constexpr float kChainMinMoveM = 1.0f;  // a hop that moved less made no progress

// Regions next to the player's, joined without a door, whose centre is this
// close count as seen: a few steps of the same open space are not a place to
// walk to. The Endar Spire bridge is cut into many such slivers, and walking
// to each one in turn sent the player back and forth (2026-11-08). Loot and
// creatures are tracked on their own, so nothing worth finding is lost.
constexpr float kSeenM       = 6.0f;
constexpr DWORD kSeenEveryMs = 500;
DWORD g_seenAt = 0;

void TickExploreChain();
void MarkNearbySeen();

constexpr float kFar = 1e30f;

// A way-out door the graph cannot route to still counts when it is this
// close in a straight line.
constexpr float kExitDoorNearM = 40.0f;

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
    // Everything standing in the region, heard before or not: a sighted
    // player sees the whole room on walking in. (The , / . listing would
    // limit this to discovered objects whenever map-wide cycling is off,
    // and never holds ground items at all.)
    Vector player;
    if (!acc::engine::GetPlayerPosition(player)) return std::string();
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
        const float dx = pos.x - player.x;
        const float dy = pos.y - player.y;
        const float dist = std::sqrt(dx * dx + dy * dy);
        if (dist > kHereRadiusM) continue;
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
        found.push_back(Entry{dist, name});
        // Heard now: the , / . cycles may list it from here on.
        acc::discovery::Record(obj);
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

// ---------------------------------------------------------------------------
// The region graph seen from the player: shortest walks between region
// centres, once with locked doors as walls and once without. Shared by the
// overview, the routes to targets in other regions and the door lines.

struct Graph {
    void*                       area   = nullptr;
    Vector                      player = {0.0f, 0.0f, 0.0f};
    std::vector<ClusterSummary> nodes;
    std::vector<ClusterLink>    links;
    std::vector<ExitMark>       exits;   // of the region under the player
    std::set<int>               linked;  // regions sharing a link with start
    int                         underPlayer = kClusterIdNone;
    int                         start       = kClusterIdNone;
    int                         si          = -1;  // start's index in nodes
    std::vector<float>          distOpen, distAll;
    std::vector<int>            hopOpen, hopAll;

    int IndexOf(int id) const {
        for (size_t i = 0; i < nodes.size(); ++i) {
            if (nodes[i].id == id) return static_cast<int>(i);
        }
        return -1;
    }
};

float Dist2D(const Vector& a, const Vector& b) {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

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
            (std::max)(0.5f, Dist2D(g.nodes[a].centre, g.nodes[b].centre));
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

struct Route {
    enum Kind { Unknown, Here, Open, Locked, Unreachable };
    Kind        kind  = Unknown;
    std::string via;        // exit of the player's region to take, may be empty
    int         walkM = 0;  // Open only, rounded to kWalkRoundM
};

int RoundWalk(float m) {
    const int step = static_cast<int>(kWalkRoundM);
    const int r = static_cast<int>(m / kWalkRoundM + 0.5f) * step;
    return r < step ? step : r;
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
        float m = Dist2D(g.player, g.nodes[hop].centre) +
                  (g.distOpen[t] - g.distOpen[hop]) +
                  Dist2D(g.nodes[t].centre, targetPos);
        m = (std::max)(m, Dist2D(g.player, targetPos));
        r.walkM = RoundWalk(m);
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
        if (!l.viaDoor || Dist2D(l.doorPos, doorPos) > kDoorMatchM) continue;
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
        const float len = Dist2D(g.player, doorPos);
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
        Dist2D(info.centre, doorPos) >= 1.0f) {
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
    return out;
}

// Loot left: containers holding items, and items on the ground, nearest
// first (straight line). Every such object in the area, whether or not the
// player has heard it yet: the , / . cycles' discovery tier is the wrong
// filter here (a footlocker not yet walked past is exactly what "left to
// search" is for, and ground items are never in that tier at all). Callers
// decide what of it to speak; `known` says whether the cycles would list it.
void CollectLoot(std::vector<LootSpot>& out) {
    out.clear();
    Vector player;
    if (!acc::engine::GetPlayerPosition(player)) return;
    void* area = acc::engine::GetCurrentArea();
    if (!area) return;
    const bool mapWide = acc::menus::modsettings::GetToggle(
        acc::menus::modsettings::Option::ExtendedCycling);
    acc::engine::AreaObjectIterator it(area);
    while (void* obj = it.Next()) {
        CycleCategory cat;
        if (acc::filter::ObjectMatches(obj, CycleCategory::Container)) {
            cat = CycleCategory::Container;
            if (acc::engine::GetContainerItemCount(obj) <= 0) continue;
        } else if (acc::filter::ObjectMatches(obj, CycleCategory::Item)) {
            cat = CycleCategory::Item;
        } else {
            continue;
        }
        Vector pos;
        if (!acc::engine::GetObjectPosition(obj, pos)) continue;
        char name[160] = {0};
        if (!acc::narration::GetSpokenName(obj, cat, name, sizeof(name)) ||
            !name[0]) {
            continue;
        }
        LootSpot s;
        s.obj      = obj;
        s.handle   = acc::engine::GetObjectHandle(obj);
        s.name     = name;
        s.pos      = pos;
        s.distance = Dist2D(pos, player);
        s.walk     = s.distance;
        s.known    = mapWide || acc::discovery::IsDiscovered(obj);
        out.push_back(std::move(s));
    }
    std::stable_sort(out.begin(), out.end(),
                     [](const LootSpot& a, const LootSpot& b) {
                         return a.distance < b.distance;
                     });
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

void FlushReminder() {
    if (g_noWayPending &&
        static_cast<LONG>(GetTickCount() - g_noWayAtMs) >= 0) {
        g_noWayPending = false;
        Vector unused;
        if (acc::engine::GetPlayerPosition(unused) &&
            !acc::engine::IsForegroundUiBlocking()) {
            const char* line = acc::strings::Get(acc::strings::Id::RouteNoWayLocked);
            prism::Speak(line, /*interrupt=*/false);
            acclog::Write("ExitMemory", "no way: %s", line);
        }
    }
    if (!g_reminderPending) return;
    if (static_cast<LONG>(GetTickCount() - g_reminderAtMs) < 0) return;
    g_reminderPending = false;
    Vector unused;
    if (!acc::engine::GetPlayerPosition(unused)) return;
    // A conversation or a panel took over meanwhile: the moment has passed.
    if (acc::engine::IsForegroundUiBlocking()) return;
    prism::Speak(g_reminderText.c_str(), /*interrupt=*/false);
    acclog::Write("ExitMemory", "loot reminder: %s", g_reminderText.c_str());
}

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

// Any door (open or closed) standing within kDoorMatchM of `p`, or null.
void* DoorNear(void* area, const Vector& p) {
    acc::engine::AreaObjectIterator it(area);
    while (void* obj = it.Next()) {
        if (acc::engine::GetObjectKind(obj) !=
            static_cast<int>(acc::engine::GameObjectKind::Door)) {
            continue;
        }
        Vector q;
        if (!acc::engine::GetObjectPosition(obj, q)) continue;
        const float dx = q.x - p.x;
        const float dy = q.y - p.y;
        if (dx * dx + dy * dy <= kDoorMatchM * kDoorMatchM) return obj;
    }
    return nullptr;
}

}  // namespace

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
    g_chain          = ExploreChain{};
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
    FlushReminder();
    TickExploreChain();
    MarkNearbySeen();
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

    Graph g;
    if (BuildGraph(area, player, g)) {
        o.haveGraph = true;

        int target = -1;
        for (size_t i = 0; i < g.nodes.size(); ++i) {
            // A doorway sliver of an area door is not a place to explore
            // (the region in front of the door stands for it).
            if (TransitionSliverOwner(area, g.nodes[i].id, nullptr) !=
                g.nodes[i].id) {
                continue;
            }
            const bool visited = g_visited.count(g.nodes[i].id) != 0;
            const bool reachable = g.distAll[i] < kFar;
            if (visited || reachable) ++o.total;
            if (visited) ++o.explored;
            if (visited || !reachable || static_cast<int>(i) == g.si) continue;
            if (g.distOpen[i] >= kFar) {
                ++o.lockedAway;
            } else if (target < 0 || g.distOpen[i] < g.distOpen[target]) {
                target = static_cast<int>(i);
            }
        }

        if (target >= 0 && g.hopOpen[target] >= 0) {
            o.haveTarget = true;
            o.targetId   = g.nodes[target].id;
            const auto landmarks = LandmarkRegions(area);
            o.targetWhat = Destination(area, g.nodes[target].id, landmarks,
                                       std::string());
            const int hop = g.hopOpen[target];
            const int hopId = g.nodes[hop].id;
            o.stepPos = g.nodes[hop].centre;
            // The closed door on the link into the first region, if any:
            // walking there means opening it.
            for (const ClusterLink& l : g.links) {
                const bool thisLink = (l.a == g.start && l.b == hopId) ||
                                      (l.b == g.start && l.a == hopId);
                if (!thisLink || !l.viaDoor) continue;
                o.stepDoor = ClosedDoorNear(area, l.doorPos);
                break;
            }
            // The exit of the current region that leads to the first region
            // on the way.
            Vector viaPos{};
            o.via = ExitToward(g, hop, &viaPos);
            o.targetPos = o.via.empty() ? g.nodes[hop].centre : viaPos;
        }

        // Nothing left to walk to, but unexplored regions behind locked
        // doors: the nearest locked door with a reachable region on one side
        // and an unexplored one on the other. 2026-11-08, Endar Spire bridge:
        // the overview only said "7 more behind locked doors", the player
        // never tried Door 1 (which opens on the attempt) and was stuck.
        if (target < 0 && o.lockedAway > 0) {
            float best = kFar;
            for (const ClusterLink& l : g.links) {
                if (!l.viaDoor) continue;
                const int a = g.IndexOf(l.a);
                const int b = g.IndexOf(l.b);
                if (a < 0 || b < 0) continue;
                int nearI = -1;
                int farI  = -1;
                if (g.distOpen[a] < kFar && g.distOpen[b] >= kFar) {
                    nearI = a;
                    farI  = b;
                } else if (g.distOpen[b] < kFar && g.distOpen[a] >= kFar) {
                    nearI = b;
                    farI  = a;
                } else {
                    continue;
                }
                if (g_visited.count(g.nodes[farI].id)) continue;
                void* door = ClosedDoorNear(area, l.doorPos);
                if (!door || !acc::engine::IsDoorLocked(door) ||
                    acc::engine::IsDoorCosmetic(door)) {
                    continue;
                }
                Vector dp = l.doorPos;
                acc::engine::GetObjectPosition(door, dp);
                const float walk =
                    (nearI == g.si)
                        ? Dist2D(player, dp)
                        : g.distOpen[nearI] + Dist2D(g.nodes[nearI].centre, dp);
                if (walk < best) {
                    best          = walk;
                    o.tryDoor     = door;
                    o.tryDoorPos  = dp;
                }
            }
        }

        // Still nothing: the nearest door the graph joins nothing through,
        // usually the way out of the area. 2026-11-08, Taris hideout: one
        // region, explored, and the exit door "To Apartment Complex" was
        // named nowhere (its far side was folded into the same region and
        // the door carries no transition text), so the player was stuck.
        if (target < 0 && !o.tryDoor) {
            float best = kFar;
            acc::engine::AreaObjectIterator it(area);
            while (void* obj = it.Next()) {
                if (acc::engine::GetObjectKind(obj) !=
                    static_cast<int>(acc::engine::GameObjectKind::Door)) {
                    continue;
                }
                if (acc::engine::IsDoorCosmetic(obj)) continue;
                Vector dp;
                if (!acc::engine::GetObjectPosition(obj, dp)) continue;
                int nearId = kClusterIdNone;
                int farId  = kClusterIdNone;
                if (DoorSides(g, dp, nearId, farId)) continue;
                const float d = Dist2D(player, dp);
                const Route r = RouteTo(g, dp);
                const bool reachable =
                    r.kind == Route::Here || r.kind == Route::Open ||
                    (r.kind == Route::Unknown && d <= kExitDoorNearM);
                if (!reachable) continue;
                const float walk =
                    r.kind == Route::Open ? static_cast<float>(r.walkM) : d;
                if (walk < best) {
                    best          = walk;
                    o.exitDoor    = obj;
                    o.exitDoorPos = dp;
                }
            }
        }
    }

    // Loot left, with the way to each spot in another region. Reachable
    // spots first, nearest walk first; what only lies behind locked doors
    // (or that the graph has no link to) goes last.
    CollectLoot(o.loot);
    if (o.haveGraph) {
        for (LootSpot& s : o.loot) {
            const Route r = RouteTo(g, s.pos);
            s.way = RouteText(r);
            if (r.kind == Route::Open) s.walk = static_cast<float>(r.walkM);
            s.reachable = r.kind != Route::Locked && r.kind != Route::Unreachable;
            // Not heard yet and not reachable now (behind a locked door, or
            // somewhere the graph has no way to): leave it out, it would
            // only name what lies ahead in the story.
            if (!s.known) {
                const bool nearUnknown =
                    r.kind == Route::Unknown && s.distance <= kHereRadiusM;
                s.keep = r.kind == Route::Here || r.kind == Route::Open ||
                         nearUnknown;
            }
        }
    } else {
        for (LootSpot& s : o.loot) s.keep = s.known || s.distance <= kHereRadiusM;
    }
    o.loot.erase(std::remove_if(o.loot.begin(), o.loot.end(),
                                [](const LootSpot& s) { return !s.keep; }),
                 o.loot.end());
    if (o.haveGraph) {
        std::stable_sort(o.loot.begin(), o.loot.end(),
                         [](const LootSpot& a, const LootSpot& b) {
                             if (a.reachable != b.reachable) return a.reachable;
                             return a.walk < b.walk;
                         });
    }
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
        if (!o.haveTarget && o.tryDoor) {
            char name[192] = {0};
            if (acc::narration::GetSpokenName(o.tryDoor, CycleCategory::Door,
                                              name, sizeof(name)) &&
                name[0]) {
                std::snprintf(buf, sizeof(buf),
                              acc::strings::Get(Id::FmtOverviewTryDoor),
                              Located(name, o.tryDoorPos).c_str());
                parts.push_back(buf);
            }
        } else if (!o.haveTarget && o.exitDoor) {
            char name[192] = {0};
            if (acc::narration::GetSpokenName(o.exitDoor, CycleCategory::Door,
                                              name, sizeof(name)) &&
                name[0]) {
                std::snprintf(buf, sizeof(buf),
                              acc::strings::Get(Id::FmtOverviewWayOut),
                              Located(name, o.exitDoorPos).c_str());
                parts.push_back(buf);
            }
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
            if (!s.way.empty()) spots += ", " + s.way;
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

std::string CurrentRegionText() {
    Vector player;
    if (!acc::engine::GetPlayerPosition(player)) return std::string();
    void* area = acc::engine::GetCurrentArea();
    if (!area) return std::string();
    return RegionLabelAt(area, player);
}

void PollOverviewHotkey() {
    if (!acc::hotkeys::Pressed(acc::hotkeys::Action::ExploreOverview)) return;
    Vector unused;
    if (!acc::engine::GetPlayerPosition(unused)) return;
    if (acc::engine::IsForegroundUiBlocking()) return;
    Overview o;
    if (!CollectOverview(o)) return;
    // The loot just named is now known: let the , / . cycles find it too.
    int named = 0;
    for (const LootSpot& s : o.loot) {
        if (named++ == kLootSpoken) break;
        if (s.obj) acc::discovery::Record(s.obj);
    }
    // Where am I first, then the area as a whole.
    std::string text = CurrentRegionText();
    if (!text.empty() && text.back() != '.') text += '.';
    if (!text.empty()) text += ' ';
    text += OverviewText(o);
    prism::Speak(text.c_str(), /*interrupt=*/true);
    acclog::Write("ExitMemory", "overview: %s", text.c_str());
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
        if (acc::engine::DoorHasTransitionDest(obj)) out.clear();
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

bool UnreachableFlags(void* const* objs, const Vector* positions, int count,
                      bool* out) {
    if (!positions || !out || count <= 0) return false;
    if (acc::view_mode::IsActive()) return false;
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

void WarnIfBehindLock(void* target) {
    if (!target || acc::view_mode::IsActive()) return;
    Vector pos;
    if (!acc::engine::GetObjectPosition(target, pos)) return;
    bool behind = false;
    void* objs[1] = {target};
    if (!UnreachableFlags(objs, &pos, 1, &behind) || !behind) return;
    // Only behind a lock, not merely without a known link: that much the
    // graph can tell for sure (an unlinked region may still be reached by a
    // lift or a scene).
    Vector player;
    if (!acc::engine::GetPlayerPosition(player)) return;
    Graph g;
    if (!BuildGraph(acc::engine::GetCurrentArea(), player, g)) return;
    int t = -1;
    if (acc::filter::ObjectMatches(target, CycleCategory::Door)) {
        int nearId = kClusterIdNone;
        int farId  = kClusterIdNone;
        if (DoorSides(g, pos, nearId, farId)) t = g.IndexOf(nearId);
    }
    if (t < 0) t = g.IndexOf(RegionAt(g.area, pos));
    if (t < 0 || g.distAll[t] >= kFar) return;
    g_noWayPending = true;
    g_noWayAtMs    = GetTickCount() + kNoWayDelayMs;
    acclog::Write("ExitMemory", "no way queued: target 0x%08x behind a lock",
                  acc::engine::GetObjectHandle(target));
}

void RemindLootOnLeaving(void* target) {
    using acc::strings::Id;
    if (!target) return;
    // Not leaving through a door that is locked shut.
    if (acc::filter::ObjectMatches(target, CycleCategory::Door) &&
        acc::engine::IsDoorLocked(target) && !acc::engine::IsDoorOpen(target)) {
        return;
    }
    const uint32_t door = acc::engine::GetObjectHandle(target);
    const DWORD now = GetTickCount();
    if (door != 0 && door == g_reminderDoor &&
        now - g_reminderDoorMs < kReminderRepeatMs) {
        return;
    }
    Vector player;
    if (!acc::engine::GetPlayerPosition(player)) return;
    void* area = acc::engine::GetCurrentArea();
    if (!area) return;
    const int here = (area == g_area && g_current >= 0) ? g_current
                                                        : RegionAt(area, player);
    if (here < 0) return;

    std::vector<LootSpot> loot;
    CollectLoot(loot);
    std::string list;
    int named = 0;
    int total = 0;
    for (const LootSpot& s : loot) {
        if (RegionAt(area, s.pos) != here) continue;
        ++total;
        if (named == kReminderNamed) continue;
        if (!list.empty()) list += "; ";
        list += s.name;
        ++named;
    }
    if (total == 0) return;
    if (total > named) {
        char more[64];
        std::snprintf(more, sizeof(more),
                      acc::strings::Get(Id::FmtRegionHereMore), total - named);
        list += "; ";
        list += more;
    }
    char buf[640];
    std::snprintf(buf, sizeof(buf), acc::strings::Get(Id::FmtLootReminder),
                  list.c_str());
    g_reminderText    = buf;
    g_reminderPending = true;
    g_reminderAtMs    = now + kReminderDelayMs;
    g_reminderDoor    = door;
    g_reminderDoorMs  = now;
    acclog::Write("ExitMemory", "loot reminder queued (region %d, door 0x%08x): %s",
                  here, door, buf);
}

namespace {

void StopChain(const char* why) {
    if (!g_chain.active) return;
    g_chain.active = false;
    acclog::Write("ExitMemory", "explore walk: stop after %d hop(s), goal %d (%s)",
                  g_chain.hops, g_chain.goal, why);
}

// One hop of the explore walk: from the player's region to the next region on
// the open way to the goal — through the closed door on that link (walked to
// and opened like Shift+- on it) or to the next region's centre. False when
// there is no open way or no walk started.
bool ChainStep() {
    Vector player;
    if (!acc::engine::GetPlayerPosition(player)) return false;
    void* area = acc::engine::GetCurrentArea();
    Graph g;
    if (!BuildGraph(area, player, g)) return false;
    const int t = g.IndexOf(g_chain.goal);
    if (t < 0 || g.si < 0 || t == g.si) return false;
    if (g.distOpen[t] >= kFar || g.hopOpen[t] < 0) return false;
    const int hop   = g.hopOpen[t];
    const int hopId = g.nodes[hop].id;
    void* door = nullptr;
    for (const ClusterLink& l : g.links) {
        const bool thisLink = (l.a == g.start && l.b == hopId) ||
                              (l.b == g.start && l.a == hopId);
        if (!thisLink || !l.viaDoor) continue;
        door = ClosedDoorNear(area, l.doorPos);
        break;
    }
    uint32_t before = 0;
    acc::guidance::LastApproachOutcome(&before);
    if (door) {
        acc::narrated_target::Stamp(door, acc::engine::GetObjectHandle(door));
        acc::cycle_input::WalkToNarratedTarget();
    } else {
        if (!acc::guidance::WalkTo(g.nodes[hop].centre)) return false;
        acc::guidance::ApproachArm arm;
        arm.owner = acc::guidance::ApproachOwner::Cycle;
        std::snprintf(arm.name, sizeof(arm.name), "%s", g_chain.name.c_str());
        arm.targetPos    = g.nodes[hop].centre;
        arm.speakBlocked = true;
        acc::guidance::ArmApproach(arm);
    }
    uint32_t serial = 0;
    acc::guidance::LastApproachOutcome(&serial);
    if (serial == before) return false;  // nothing was dispatched
    g_chain.doorTries = (door && door == g_chain.door) ? g_chain.doorTries + 1 : 0;
    g_chain.door     = door;
    g_chain.serial   = serial;
    g_chain.endedAt  = 0;
    g_chain.stepFrom = player;
    acclog::Write("ExitMemory", "explore walk: hop %d region %d -> %d%s (goal %d)",
                  g_chain.hops, g.start, hopId, door ? " through a door" : "",
                  g_chain.goal);
    return true;
}

void TickExploreChain() {
    if (!g_chain.active) return;
    if (acc::guidance::IsAnyModApproachInFlight()) {
        g_chain.endedAt = 0;
        return;
    }
    uint32_t serial = 0;
    const acc::guidance::ApproachOutcome out =
        acc::guidance::LastApproachOutcome(&serial);
    if (serial != g_chain.serial) return StopChain("another walk took over");
    if (out == acc::guidance::ApproachOutcome::Blocked) return StopChain("blocked");
    if (out == acc::guidance::ApproachOutcome::Cancelled) return StopChain("cancelled");
    if (acc::engine::GetCurrentArea() != g_area) return StopChain("area changed");
    if (g_chain.goal == g_current || g_visited.count(g_chain.goal)) {
        return StopChain("arrived");
    }
    if (acc::engine::IsAnyPartyMemberInCombat()) return StopChain("combat");
    if (acc::engine::HasActiveDialogPanel() ||
        acc::engine::IsForegroundUiBlocking()) {
        return StopChain("panel open");
    }
    const DWORD now = GetTickCount();
    if (g_chain.endedAt == 0) {
        g_chain.endedAt = now;
        return;
    }
    if (now - g_chain.endedAt < kChainSettleMs) return;
    if (acc::engine::WorldIsPaused()) return;  // carries on after unpause
    Vector p;
    if (!acc::engine::GetPlayerPosition(p)) return;
    // A door hop may open the door without a step (the player stood at it);
    // any other hop that left the player where it began made no progress.
    if (!g_chain.door && Dist2D(p, g_chain.stepFrom) < kChainMinMoveM) {
        return StopChain("no progress");
    }
    if (g_chain.doorTries >= 2) return StopChain("door did not open");
    if (++g_chain.hops > kChainMaxHops) return StopChain("hop limit");
    if (!ChainStep()) {
        ClusterInfo goal;
        if (acc::room_topology::GetClusterInfo(g_area, g_chain.goal, goal)) {
            acc::guidance::SpeakWayBlocked(g_chain.name.c_str(), goal.centre);
        }
        StopChain("no way on");
    }
}

void MarkNearbySeen() {
    if (!g_area || g_current < 0) return;
    const DWORD now = GetTickCount();
    if (now - g_seenAt < kSeenEveryMs) return;
    g_seenAt = now;
    if (acc::engine::GetCurrentArea() != g_area) return;
    Vector p;
    if (!acc::engine::GetPlayerPosition(p)) return;
    std::vector<ClusterLink> links;
    acc::room_topology::GetClusterLinks(g_area, links);
    for (const ClusterLink& l : links) {
        if (l.viaDoor) continue;
        int other;
        if (l.a == g_current) other = l.b;
        else if (l.b == g_current) other = l.a;
        else continue;
        if (other < 0 || g_visited.count(other)) continue;
        ClusterInfo info;
        if (!acc::room_topology::GetClusterInfo(g_area, other, info)) continue;
        const float d = Dist2D(p, info.centre);
        if (d > kSeenM) continue;
        RecordVisit(other);
        acclog::Write("ExitMemory", "region %d seen from region %d (%.1f m)",
                      other, g_current, d);
    }
}

void WalkTowardUnexplored() {
    using acc::strings::Id;
    Overview o;
    if (!CollectOverview(o)) return;
    if (!o.haveTarget || o.targetId < 0) {
        StopChain("nothing left");
        const uint32_t handle =
            o.tryDoor ? acc::engine::GetObjectHandle(o.tryDoor) : 0;
        if (handle != 0) {
            // Only a locked door leads on: try it, exactly as Enter on it.
            // The Trask guard speaks its own reason and refuses.
            if (acc::endar::BlockCompanionAttempt(o.tryDoor)) return;
            acc::narrated_target::Stamp(o.tryDoor, handle);
            acc::interact::DispatchInteract(o.tryDoor, handle,
                                            /*forceRadial=*/false);
            // Queued behind the interact line, which names the door.
            prism::Speak(acc::strings::Get(Id::ExploreTryingDoor),
                         /*interrupt=*/false);
            acclog::Write("ExitMemory", "explore walk: nothing reachable left, "
                          "trying locked door 0x%08x", handle);
            return;
        }
        const uint32_t exitHandle =
            o.exitDoor ? acc::engine::GetObjectHandle(o.exitDoor) : 0;
        char exitName[192] = {0};
        if (exitHandle != 0 &&
            acc::narration::GetSpokenName(o.exitDoor, CycleCategory::Door,
                                          exitName, sizeof(exitName)) &&
            exitName[0]) {
            // Only a way out is left: make it the target and say so. No
            // walk: going through may load another area, so that stays
            // the player's own Enter.
            acc::narrated_target::Stamp(o.exitDoor, exitHandle);
            char line[640];
            std::snprintf(line, sizeof(line),
                          acc::strings::Get(Id::FmtExploreWayOut),
                          Located(exitName, o.exitDoorPos).c_str());
            prism::Speak(line, /*interrupt=*/true);
            acclog::Write("ExitMemory", "explore walk: way out 0x%08x -> [%s]",
                          exitHandle, line);
            return;
        }
        // Nothing reachable left: say why (all explored / behind locks).
        const std::string text = OverviewText(o);
        prism::Speak(text.c_str(), /*interrupt=*/true);
        return;
    }
    g_chain        = ExploreChain{};
    g_chain.active = true;
    g_chain.goal   = o.targetId;
    g_chain.name   = o.targetWhat.empty() ? std::string(acc::strings::Get(Id::ExitAnnoNew))
                                          : o.targetWhat;

    // One line up front: what the walk heads for and how far it is to walk.
    std::string subject = g_chain.name;
    Vector player;
    Graph g;
    if (acc::engine::GetPlayerPosition(player) &&
        BuildGraph(acc::engine::GetCurrentArea(), player, g)) {
        const int t = g.IndexOf(o.targetId);
        if (t >= 0) {
            const std::string way = RouteText(RouteToIndex(g, t, g.nodes[t].centre));
            if (!way.empty()) subject += ", " + way;
        }
    }
    if (!ChainStep()) {
        g_chain.active = false;
        char fail[256];
        std::snprintf(fail, sizeof(fail), acc::strings::Get(Id::FmtGuidingFailed),
                      g_chain.name.c_str());
        prism::Speak(fail, /*interrupt=*/true);
        return;
    }
    char buf[640];
    std::snprintf(buf, sizeof(buf), acc::strings::Get(Id::FmtWalkingTo),
                  subject.c_str());
    prism::Speak(buf, /*interrupt=*/true);
    acclog::Write("ExitMemory", "walk unexplored: %s (goal region %d)", buf,
                  o.targetId);
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

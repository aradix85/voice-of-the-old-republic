// Visited regions: kept per area in the save (ACC_VISIT_<area>), the
// area-enter and region-commit edges, regions seen from nearby, and the
// per-frame Tick that reads the save once the player has settled.

#include "exit_memory_internal.h"

namespace acc::exit_memory {

namespace {

std::string   g_varName;   // ACC_VISIT_<areaKey>, empty = not persisted

std::string g_areaName;               // display name of g_area

// Save-var reconciliation, like discovery: read only once the player
// creature has been the same pointer for a settle window (a read on the
// save-load tick races the engine restoring the var table).

void* g_settleCreature = nullptr;
int   g_settleTicks    = 0;
constexpr int    kSettleTicks     = 60;
constexpr size_t kMaxSavedRegions = 300;
// A saved region centre matches a live region whose centre is this close.
constexpr float  kSavedMatchM     = 2.5f;

// Regions next to the player's, joined without a door, whose centre is this
// close count as seen: a few steps of the same open space are not a place to
// walk to. The Endar Spire bridge is cut into many such slivers, and walking
// to each one in turn sent the player back and forth (2026-11-08). Loot and
// creatures are tracked on their own, so nothing worth finding is lost.
constexpr float kSeenM       = 6.0f;
constexpr DWORD kSeenEveryMs = 500;
DWORD g_seenAt = 0;

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

}  // namespace

void OnAreaEntered(void* area) {
    char key[64] = {0};
    if (!acc::engine::GetCurrentAreaResName(key, sizeof(key))) key[0] = '\0';
    // The area left and the spot it was left from: once settled here,
    // TickAreaMemory checks that the player arrived at a door back to it.
    g_arrival = Arrival{};
    if (g_haveLastPos && !g_areaKey.empty() && !g_areaName.empty() &&
        g_areaKey != ToLowerAscii(key)) {
        g_arrival.fromKey  = g_areaKey;
        g_arrival.fromName = g_areaName;
        g_arrival.fromPos  = g_lastPos;
        g_arrival.pending  = true;
    }
    g_haveLastPos  = false;
    g_summarySaved.clear();
    g_summaryAtMs  = 0;
    {
        char name[128] = {0};
        g_areaName = acc::engine::GetAreaDisplayName(area, name, sizeof(name))
                         ? std::string(name) : std::string();
    }
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
    TickAreaMemory();
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

}  // namespace acc::exit_memory

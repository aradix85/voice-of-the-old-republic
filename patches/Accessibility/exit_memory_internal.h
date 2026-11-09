// Internal to the exit-memory modules (exit_memory_*.cpp); no other file
// includes it. The public interface is exit_memory.h. Here: what the modules
// share: types, constants, the state defined in exit_memory_state.cpp, and
// the helpers one module calls in another. What only one file uses stays in
// that file.

#pragma once

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

using acc::filter::CycleCategory;
using acc::room_topology::ClusterInfo;
using acc::room_topology::ClusterLink;
using acc::room_topology::ClusterSummary;
using acc::room_topology::ExitMark;
using acc::room_topology::kClusterIdNone;

extern void* g_area;
extern std::string g_areaKey;  // lower-case module resref
extern std::set<int> g_visited;
extern int g_current;
extern int g_previous;

// Doors into other areas, remembered in the save: per area the spots of the
// doors the player went through and the module each led to
// (ACC_EXIT_<area> = "x,y=module;..."), and per area the map notes seen
// there (ACC_SUM_<area>). Two areas the game names alike (Taris' two "Lower
// City Apartments") are told apart by what is inside: "Matrik's Apartment"
// against "Selven's Apartment" (2026-11-09).
extern Vector g_lastPos;  // player, last tick in g_area
extern bool g_haveLastPos;

struct Arrival {
    std::string fromKey;
    std::string fromName;
    Vector      fromPos = {0.0f, 0.0f, 0.0f};
    bool        pending = false;
};
extern Arrival g_arrival;
extern DWORD g_summaryAtMs;
extern std::string g_summarySaved;

// The save variables of this area have been read (after the settle
// window in exit_memory_visits.cpp).
extern bool g_saveLoaded;

// "Here:" and the overview: objects farther than this from the player are
// not considered (a cheap pre-filter before the region test).
constexpr float kHereRadiusM = 30.0f;

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
extern ExploreChain g_chain;

constexpr float kFar = 1e30f;

// Door exits carry the door's own position (room_topology::ExitPoint). Give
// such an exit the same number the Q/E and , / . cycles use ("Door 2"), so
// "Door East" in the region label and "Door 2" in the lists are visibly the
// same door: "Door East" becomes "Door 2 East".
struct DoorName {
    Vector      pos;
    std::string base;      // what the label's noun is ("Door", "Lift")
    std::string numbered;  // base + area ordinal ("Door 2"); == base if unique
};

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

struct Route {
    enum Kind { Unknown, Here, Open, Locked, Unreachable };
    Kind        kind  = Unknown;
    std::string via;        // exit of the player's region to take, may be empty
    int         walkM = 0;  // Open only, rounded to kWalkRoundM
};

// Helpers shared across the modules, by the file that defines them.

// exit_memory_regions.cpp
std::string ToLowerAscii(const char* s);
int RegionAt(void* area, const Vector& p);
std::set<int> LinkedRegions(void* area, int self);
int RegionBehind(void* area, int self, const std::set<int>& linked, const Vector& from, const Vector& exitPos);
std::vector<std::pair<int, std::string>> LandmarkRegions(void* area);
std::vector<DoorName> NumberedDoors();
void NumberDoorExit(ExitMark& e, const std::vector<DoorName>& doors);
std::string Destination(void* area, int target, const std::vector<std::pair<int, std::string>>& landmarks, const std::string& exitText);
float Dist2D(const Vector& a, const Vector& b);
bool NearAny(const Vector& p, const std::vector<Vector>& points, float maxM);
std::string Located(const std::string& subject, const Vector& pos);

// exit_memory_doors.cpp
std::vector<Vector> LockedDoorPositions(void* area);
void* ClosedDoorNear(void* area, const Vector& p);
void* DoorNear(void* area, const Vector& p);
void* TransitionDoorNear(void* area, const Vector& p, float maxM, const std::string& destName, Vector* outPos);

// exit_memory_graph.cpp
bool BuildGraph(void* area, const Vector& player, Graph& g);
std::string ExitToward(const Graph& g, int hop, Vector* outPos);
Route RouteToIndex(const Graph& g, int t, const Vector& targetPos);
Route RouteTo(const Graph& g, const Vector& targetPos);
std::string RouteText(const Route& r);
bool DoorSides(const Graph& g, const Vector& doorPos, int& nearId, int& farId);
std::string DoorBeyond(const Graph& g, const Vector& doorPos, int* outNear);

// exit_memory_annotate.cpp
std::string BehindDoorText(void* area, int farId);

// exit_memory_area_doors.cpp
std::string TransitionDoorMemory(const Vector& doorPos);
void TickAreaMemory();

// exit_memory_overview.cpp
void FlushReminder();

// exit_memory_explore.cpp
void TickExploreChain();

}  // namespace acc::exit_memory

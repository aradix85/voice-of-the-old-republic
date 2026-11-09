// Exit memory — what lies behind each exit of the player's current region,
// whether the player has been there, which exit leads back, what is in the
// region, and an overview of the whole area.
//
// The region label ("Junction. Exits: North, Door East, Door South") tells
// the player WHERE the exits are but not WHERE THEY GO. A sighted player
// sees the corridor behind a door, remembers the way they came and notices
// the footlocker in the corner; this module gives the label the same
// information:
//
//   "Junction. Exits: North, to the Bridge, new; Door 2 East, to a
//    corridor, visited; Door 3 South, back. Here: Footlocker; Utility Droid"
//
// Per exit, at speech time:
//   * the region behind it is found by probing the nav-graph regions just
//     past the exit point, away from the current region's centre;
//   * destination: a discovered landmark inside that region ("to the
//     Bridge"), else the region's kind ("to a corridor");
//   * status: "back" when it is the region the player just came from,
//     else "visited" or "new";
//   * door exits take the area ordinal the cycles use ("Door 2 East").
// Area-transition doors probe off the walkmesh and stay unannotated (their
// label already names the destination area).
//
// "Here:" lists the creatures (not the party), usable placeables that are
// not emptied loot, and items on the ground whose position falls in the
// region — whatever the , / . cycles would list, named the same way.
//
// Visited regions are remembered per area in the save game (player script
// var ACC_VISIT_<area>, the region centres in metres), so "visited" and the
// overview survive a reload. Regions the player only passes through count
// too (OnClusterSeen), not just the ones that were announced.
//
// The overview (Ctrl+N, and the bridge's /overview): how many regions of
// the area were explored, the nearest unexplored one by walking through the
// region graph without passing a locked door, the exit of the current
// region that leads toward it, how many unexplored regions sit behind
// locked doors, and the loot containers with items and the items on the
// ground that are left, nearest first.
//
// Only the player's own region announces are annotated; the view-mode
// cursor keeps the plain label.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "engine_offsets.h"  // Vector
#include "room_topology.h"

namespace acc::exit_memory {

// Area-enter edge (after room_topology::BuildForArea). Switches to this
// area's visited set (loaded from the save once settled, see Tick) and
// forgets the current/previous region.
void OnAreaEntered(void* area);

// The player's committed region changed (transitions' stability-gated
// cluster commit). Records `to` as visited and `from` as the way back.
void OnClusterCommitted(int from, int to);

// The player has stood in region `id` for the stability window, whether or
// not it gets announced (small regions passed through never commit).
// Marks it visited only.
void OnClusterSeen(int id);

// The bit of walkmesh at a door into another area (the lift you arrive by,
// the doorway of an area door) can be a region of its own, joined to the
// rest of the area only through that door. Standing in it is standing in
// the region in front of the door: returns that region (and its centre in
// outCentre, may be null) for such a sliver, else `region` unchanged, so a
// route from the Lower City lift does not start "via Elevator 2 North, To
// Upper City". Cached briefly; cheap per frame.
int TransitionSliverOwner(void* area, int region, Vector* outCentre);

// Once per frame: loads this area's visited regions from the save once the
// player creature has settled and the region graph exists.
void Tick();

// Rewrite a player-region label with destination, visited and way-back
// words, door numbers and the "Here:" list. False (output untouched) when
// there is nothing to add. When the exits are annotated the list is
// re-joined with "; " so the commas inside an annotated exit do not read as
// extra exits.
bool Annotate(void* area, const std::string& label,
              const std::vector<acc::room_topology::ExitMark>& exits,
              std::string& outLabel);

// One thing still worth a look: a loot container that holds items, or an
// item on the ground. Named as the , / . cycle names it (game codepage).
struct LootSpot {
    void*       obj    = nullptr;
    uint32_t    handle = 0;  // server handle
    std::string name;
    Vector      pos      = {0.0f, 0.0f, 0.0f};
    float       distance = 0.0f;  // metres from the player
    // The way there when it lies in another region ("via Door 2 East, about
    // 30 metres to walk" / "behind a locked door"), empty in the player's
    // own region or when the graph cannot tell.
    std::string way;
    float       walk      = 0.0f;  // walking metres (straight line if unknown)
    bool        reachable = true;  // false: only through locked doors / no link
    bool        known     = true;  // heard before (or map-wide cycling is on)
    bool        keep      = true;  // internal: listed in the overview
};

struct Overview {
    bool        haveGraph = false;
    int         explored  = 0;     // regions visited
    int         total     = 0;     // regions the player can reach or has seen
    bool        haveTarget = false;
    int         targetId  = -1;    // region id of the nearest unexplored region
    std::string targetWhat;        // "to a corridor" / "to the Bridge"
    std::string via;               // exit of the current region toward it, may be empty
    Vector      targetPos = {0.0f, 0.0f, 0.0f};  // the via exit, else the next region's centre
    Vector      stepPos   = {0.0f, 0.0f, 0.0f};  // centre of the first region on the way
    void*       stepDoor  = nullptr;  // closed (unlocked) door on the way into it, or null
    int         lockedAway = 0;    // unexplored regions reachable only through locked doors
    // Nothing left to walk to: the nearest locked door (not set dressing)
    // standing between the reachable regions and an unexplored one. Story
    // doors often open on the attempt itself (Endar Spire), so it is the
    // door to try. Null when there is a target or no such door.
    void*       tryDoor    = nullptr;
    Vector      tryDoorPos = {0.0f, 0.0f, 0.0f};
    // Nothing left to walk to and no locked door to try: the nearest door
    // the region graph joins nothing through (a way out of the area, or a
    // door whose far side the graph does not know). Null otherwise.
    void*       exitDoor    = nullptr;
    Vector      exitDoorPos = {0.0f, 0.0f, 0.0f};
    std::vector<LootSpot> loot;    // reachable first, then by walking distance
};

// False when the player is not in the world.
bool CollectOverview(Overview& out);

// The spoken overview (game codepage, localized).
std::string OverviewText(const Overview& o);

// The player's region as the walk-in announce speaks it (with where each
// exit leads and "Here:" when known). Empty outside the world / graph.
std::string CurrentRegionText();

// Ctrl+N (Action::ExploreOverview): the current region first, then the
// overview. Called from the input poll router.
void PollOverviewHotkey();

// Extra words for an explicitly requested target line (Q/E, , / ., `-`):
//   * a door: which way it leads and what lies beyond, seen from the
//     player's side ("leads West, to a corridor, visited" / "leads South,
//     back"), plus the way there when the door is in another region;
//   * anything else in another region: the way there ("via Door 2 East,
//     about 30 metres to walk", or "behind a locked door").
// Empty when there is nothing to add (same region, no graph, view mode).
// Game codepage, localized.
std::string TargetWayInfo(void* obj, const Vector& pos);

// Enter on a door or exit (interact dispatch): when the region being left
// still holds loot, queue a short "Still here: Metal box" a beat after the
// walk line. Never blocks or replaces the walk; not repeated for the same
// door within 20 s; dropped if a conversation or panel opens meanwhile.
void RemindLootOnLeaving(void* target);

// For the , / . cycles: out[i] = true when positions[i] (objs[i], may be
// null) can be reached from the player only through a locked door, or lies
// in a region the graph has no way to. Doors count by the side the player
// walks up to. False (out untouched) without a graph or in view mode.
// Reads no cycle listings, so cycle_state may call it.
bool UnreachableFlags(void* const* objs, const Vector* positions, int count,
                      bool* out);

// The area's regions as the overview sees them, for the bridge's /rooms.
struct RegionSummary {
    int    id      = -1;
    Vector centre  = {0.0f, 0.0f, 0.0f};
    const char* kind = "";  // "corridor" / "junction" / "dead end" / "room"
    bool   visited = false;
    bool   here    = false;   // the player's region
    bool   reachable = false; // without passing a locked door
    bool   behindLock = false;  // only through a locked door
    float  walk    = 0.0f;    // centre-to-centre metres from the player's region
};
// False without a graph. `links` gets each region pair once (a < b).
bool ListRegions(std::vector<RegionSummary>& out,
                 std::vector<std::pair<int, int>>& links);

// Enter on something that lies only behind a locked door: a beat after the
// walk line, "No way there now: it is behind a locked door." The dispatch
// itself is never held back.
void WarnIfBehindLock(void* target);

// Ctrl+Shift+N (ExploreWalkUnexplored): walk the whole way to the nearest
// unexplored region, region by region, opening closed doors on the way. One
// line up front (what and how far); the regions passed announce themselves as
// usual. When nothing unexplored is reachable but a locked door leads to an
// unexplored region, it tries that door like Enter on it. Stops on arrival,
// on manual movement or any other walk, a blocked
// step, combat or an opened panel. Ctrl+L (ExploreWalkLoot): walk to the
// nearest container with items or item on the ground and use it, like Shift+-
// on it.
void PollWalkHotkeys();

}  // namespace acc::exit_memory

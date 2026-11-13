# exit_memory.h (223 lines)

Source: `patches/Accessibility/exit_memory.h`

Exit memory — what lies behind each exit of the player's current region, whether the player has been there, which exit leads back, what is in the region, and an overview of the whole area.

The region label ("Junction. Exits: North, Door East, Door South") tells the player WHERE the exits are but not WHERE THEY GO. A sighted player sees the corridor behind a door, remembers the way they came and notices the footlocker in the corner; this module gives the label the same information:

"Junction. Exits: North, to the Bridge, new; Door 2 East, to a corridor, visited; Door 3 South, back. Here: Footlocker; Utility Droid"

Per exit, at speech time: * the region behind it is found by probing the nav-graph regions just past the exit point, away from the current region's centre; * destination: a discovered landmark inside that region ("to the Bridge"), else the region's kind ("to a corridor"); * status: "back" when it is the region the player just came from, else "visited" or "new"; * door exits take the area ordinal the cycles use ("Door 2 East"). Area-transition doors probe off the walkmesh and stay unannotated (their label already names the destination area).

"Here:" lists the creatures (not the party), usable placeables that are not emptied loot, and items on the ground whose position falls in the region — whatever the , / . cycles would list, named the same way.

Visited regions are remembered per area in the save game (player script var ACC_VISIT_<area>, the region centres in metres), so "visited" and the overview survive a reload. Regions the player only passes through count too (OnClusterSeen), not just the ones that were announced.

The overview (Ctrl+N, and the bridge's /overview): how many regions of the area were explored, the nearest unexplored one by walking through the region graph without passing a locked door, the exit of the current region that leads toward it, how many unexplored regions sit behind locked doors, and the loot containers with items and the items on the ground that are left, nearest first.

## Declarations (in source order)

- L55 `void OnAreaEntered(void* area)`: Area-enter edge (after room_topology::BuildForArea).
- L59 `void OnClusterCommitted(int from, int to)`: The player's committed region changed (transitions' stability-gated cluster commit).
- L64 `void OnClusterSeen(int id)`: The player has stood in region `id` for the stability window, whether or not it gets announced (small regions passed through never commit).
- L73 `int TransitionSliverOwner(void* area, int region, Vector* outCentre)`: The bit of walkmesh at a door into another area (the lift you arrive by, the doorway of an area door) can be a region of its own, joined to the rest of the area only through that door.
- L77 `void Tick()`: Once per frame: loads this area's visited regions from the save once the player creature has settled and the region graph exists.
- L84 `bool Annotate(void* area, const std::string& label, const std::vector<acc::room_topology::ExitMark>& exits, std::string& outLabel)`: Rewrite a player-region label with destination, visited and way-back words, door numbers and the "Here:" list.
- L90 `struct LootSpot`: One thing still worth a look: a loot container that holds items, or an item on the ground.
- L106 `struct Overview`
- L133 `bool CollectOverview(Overview& out)`: False when the player is not in the world.
- L136 `std::string OverviewText(const Overview& o)`: The spoken overview (game codepage, localized).
- L140 `std::string CurrentRegionText()`: The player's region as the walk-in announce speaks it (with where each exit leads and "Here:" when known).
- L144 `void PollOverviewHotkey()`: Ctrl+N (Action::ExploreOverview): the current region first, then the overview.
- L154 `std::string TargetWayInfo(void* obj, const Vector& pos)`: Extra words for an explicitly requested target line (Q/E, , / ., `-`): * a door: which way it leads and what lies beyond, seen from the player's side ("leads West, to a corridor, visited" / "leads South, back"), plus ...
- L160 `void RemindLootOnLeaving(void* target)`: Enter on a door or exit (interact dispatch): when the region being left still holds loot, queue a short "Still here: Metal box" a beat after the walk line.
- L167 `bool UnreachableFlags(void* const* objs, const Vector* positions, int count, bool* out)`: For the , / .
- L171 `struct RegionSummary`: The area's regions as the overview sees them, for the bridge's /rooms.
- L182 `bool ListRegions(std::vector<RegionSummary>& out, std::vector<std::pair<int, int>>& links)`: False without a graph.
- L188 `void WarnIfBehindLock(void* target)`: Enter on something that lies only behind a locked door: a beat after the walk line, "No way there now: it is behind a locked door." The dispatch itself is never held back.
- L193 `bool BeyondDoor(const Vector& doorPos, Vector& out)`: Where a walk through a door ends: the centre of the region beyond it, seen from the player.
- L202 `bool WalkThenInteract(void* target, uint32_t handle, const char* name)`: Enter or Ctrl+L on anything in another region with a closed door on the way, or on an enemy round a corner: walk there region by region, opening the doors, and act on the target on arrival, as Enter on it does.
- L209 `bool RescueWalk(void* target, const char* name, bool act)`: The same walk as a net under every walk the approach tracker watches: the engine did not walk to `target`, whatever the reason.
- L220 `void PollWalkHotkeys()`: Ctrl+Shift+N (ExploreWalkUnexplored): walk the whole way to the nearest unexplored region, region by region, opening closed doors on the way.

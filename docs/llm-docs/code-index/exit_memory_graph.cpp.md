# exit_memory_graph.cpp (483 lines)

Source: `patches/Accessibility/exit_memory_graph.cpp`

The region graph and walking routes: shortest walks with and without locked doors, the exit to take, route words, the two sides of a door, doorway slivers of area doors, reachability flags for the cycles, /rooms.

## Declarations (in source order)

- L42 `bool BuildGraph(void* area, const Vector& player, Graph& g)`
- L102 `std::string ExitToward(const Graph& g, int hop, Vector* outPos)`: The exit of the player's region that leads into region index `hop`, numbered like the cycles ("Door 2 East").
- L128 `Route RouteToIndex(const Graph& g, int t, const Vector& targetPos)`: The way from the player to `targetPos`, which lies in region index `t`.
- L150 `int IndexAt(const Graph& g, const Vector& pos)`
- L157 `int TargetIndex(const Graph& g, void* obj, const Vector& pos)`
- L168 `Route RouteTo(const Graph& g, const Vector& targetPos)`
- L172 `std::string RouteText(const Route& r)`
- L202 `bool DoorSides(const Graph& g, const Vector& doorPos, int& nearId, int& farId)`: Where a door leads, from the player's side: the compass direction through it and the region beyond ("leads West, to a corridor, visited"; "leads South, back").
- L275 `std::string DoorBeyond(const Graph& g, const Vector& doorPos, int* outNear)`
- L322 `int TransitionSliverOwner(void* area, int region, Vector* outCentre)`
- L369 `bool UnreachableFlags(void* const* objs, const Vector* positions, int count, bool* out)`
- L387 `Vector RegionWalkPoint(void* area, int id, const Vector& centre)`
- L417 `bool BeyondDoor(const Vector& doorPos, Vector& out, int* outRegion)`
- L434 `void* OnwardDoor(int region, const Vector& passedDoorPos)`
- L451 `bool ListRegions(std::vector<RegionSummary>& out, std::vector<std::pair<int, int>>& links)`

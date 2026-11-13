# exit_memory_internal.h (189 lines)

Source: `patches/Accessibility/exit_memory_internal.h`

Internal to the exit-memory modules (exit_memory_*.cpp); no other file includes it. The public interface is exit_memory.h. Here: what the modules share: types, constants, the state defined in exit_memory_state.cpp, and the helpers one module calls in another. What only one file uses stays in that file.

## Declarations (in source order)

- L50 `struct Arrival`
- L79 `struct ExploreChain`: Ctrl+Shift+N walks the whole way to the nearest unexplored region, one region hop after another: each hop is a short walk to the next region's centre, which the engine reliably completes (a single long walk across sev...
- L101 `struct DoorName`: Door exits carry the door's own position (room_topology::ExitPoint).
- L111 `struct Graph`
- L136 `struct Route`
- L147 `int RegionAt(void* area, const Vector& p, bool throughWalls = false)`: exit_memory_regions.cpp throughWalls: the nearest region, also when a wall stands in between.
- L148 `std::set<int> LinkedRegions(void* area, int self)`
- L149 `int RegionBehind(void* area, int self, const std::set<int>& linked, const Vector& from, const Vector& exitPos)`
- L150 `std::vector<std::pair<int, std::string>> LandmarkRegions(void* area)`
- L151 `std::vector<DoorName> NumberedDoors()`
- L152 `void NumberDoorExit(ExitMark& e, const std::vector<DoorName>& doors)`
- L153 `std::string Destination(void* area, int target, const std::vector<std::pair<int, std::string>>& landmarks, const std::string& exitText)`
- L154 `bool NearAny(const Vector& p, const std::vector<Vector>& points, float maxM)`
- L155 `std::string Located(const std::string& subject, const Vector& pos)`
- L158 `std::vector<Vector> LockedDoorPositions(void* area)`: exit_memory_doors.cpp
- L159 `void* ClosedDoorNear(void* area, const Vector& p)`
- L160 `void* DoorNear(void* area, const Vector& p)`
- L161 `void* TransitionDoorNear(void* area, const Vector& p, float maxM, const std::string& destName, Vector* outPos)`
- L164 `bool BuildGraph(void* area, const Vector& player, Graph& g)`: exit_memory_graph.cpp
- L165 `std::string ExitToward(const Graph& g, int hop, Vector* outPos)`
- L168 `int IndexAt(const Graph& g, const Vector& pos)`: The graph index of the region `pos` lies in; for a spot off the walkable floor, of the region nearest to it.
- L169 `Route RouteToIndex(const Graph& g, int t, const Vector& targetPos)`
- L170 `Route RouteTo(const Graph& g, const Vector& targetPos)`
- L171 `std::string RouteText(const Route& r)`
- L172 `bool DoorSides(const Graph& g, const Vector& doorPos, int& nearId, int& farId)`
- L173 `std::string DoorBeyond(const Graph& g, const Vector& doorPos, int* outNear)`
- L176 `std::string BehindDoorText(void* area, int farId)`: exit_memory_annotate.cpp
- L179 `std::string TransitionDoorMemory(const Vector& doorPos)`: exit_memory_area_doors.cpp
- L180 `void TickAreaMemory()`
- L183 `void FlushReminder()`: exit_memory_overview.cpp
- L186 `void TickExploreChain()`: exit_memory_explore.cpp

Also 22 constants or variables at namespace scope.

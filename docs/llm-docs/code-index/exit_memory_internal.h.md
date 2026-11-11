# exit_memory_internal.h (182 lines)

Source: `patches/Accessibility/exit_memory_internal.h`

Internal to the exit-memory modules (exit_memory_*.cpp); no other file includes it. The public interface is exit_memory.h. Here: what the modules share: types, constants, the state defined in exit_memory_state.cpp, and the helpers one module calls in another. What only one file uses stays in that file.

## Declarations (in source order)

- L50 `struct Arrival`
- L79 `struct ExploreChain`: Ctrl+Shift+N walks the whole way to the nearest unexplored region, one region hop after another: each hop is a short walk to the next region's centre, which the engine reliably completes (a single long walk across sev...
- L99 `struct DoorName`: Door exits carry the door's own position (room_topology::ExitPoint).
- L109 `struct Graph`
- L133 `struct Route`
- L143 `int RegionAt(void* area, const Vector& p)`: exit_memory_regions.cpp
- L144 `std::set<int> LinkedRegions(void* area, int self)`
- L145 `int RegionBehind(void* area, int self, const std::set<int>& linked, const Vector& from, const Vector& exitPos)`
- L146 `std::vector<std::pair<int, std::string>> LandmarkRegions(void* area)`
- L147 `std::vector<DoorName> NumberedDoors()`
- L148 `void NumberDoorExit(ExitMark& e, const std::vector<DoorName>& doors)`
- L149 `std::string Destination(void* area, int target, const std::vector<std::pair<int, std::string>>& landmarks, const std::string& exitText)`
- L150 `bool NearAny(const Vector& p, const std::vector<Vector>& points, float maxM)`
- L151 `std::string Located(const std::string& subject, const Vector& pos)`
- L154 `std::vector<Vector> LockedDoorPositions(void* area)`: exit_memory_doors.cpp
- L155 `void* ClosedDoorNear(void* area, const Vector& p)`
- L156 `void* DoorNear(void* area, const Vector& p)`
- L157 `void* TransitionDoorNear(void* area, const Vector& p, float maxM, const std::string& destName, Vector* outPos)`
- L160 `bool BuildGraph(void* area, const Vector& player, Graph& g)`: exit_memory_graph.cpp
- L161 `std::string ExitToward(const Graph& g, int hop, Vector* outPos)`
- L162 `Route RouteToIndex(const Graph& g, int t, const Vector& targetPos)`
- L163 `Route RouteTo(const Graph& g, const Vector& targetPos)`
- L164 `std::string RouteText(const Route& r)`
- L165 `bool DoorSides(const Graph& g, const Vector& doorPos, int& nearId, int& farId)`
- L166 `std::string DoorBeyond(const Graph& g, const Vector& doorPos, int* outNear)`
- L169 `std::string BehindDoorText(void* area, int farId)`: exit_memory_annotate.cpp
- L172 `std::string TransitionDoorMemory(const Vector& doorPos)`: exit_memory_area_doors.cpp
- L173 `void TickAreaMemory()`
- L176 `void FlushReminder()`: exit_memory_overview.cpp
- L179 `void TickExploreChain()`: exit_memory_explore.cpp

Also 22 constants or variables at namespace scope.

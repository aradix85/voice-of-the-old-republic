# exit_memory_internal.h (199 lines)

Source: `patches/Accessibility/exit_memory_internal.h`

Internal to the exit-memory modules (exit_memory_*.cpp); no other file includes it. The public interface is exit_memory.h. Here: what the modules share: types, constants, the state defined in exit_memory_state.cpp, and the helpers one module calls in another. What only one file uses stays in that file.

## Declarations (in source order)

- L50 `struct Arrival`
- L79 `struct ExploreChain`: Ctrl+Shift+N walks the whole way to the nearest unexplored region, one region hop after another: each hop is a short walk to the next region's centre, which the engine reliably completes (a single long walk across sev...
- L102 `struct DoorName`: Door exits carry the door's own position (room_topology::ExitPoint).
- L112 `struct Graph`
- L137 `struct Route`
- L148 `int RegionAt(void* area, const Vector& p, bool throughWalls = false)`: exit_memory_regions.cpp throughWalls: the nearest region, also when a wall stands in between.
- L149 `std::set<int> LinkedRegions(void* area, int self)`
- L150 `int RegionBehind(void* area, int self, const std::set<int>& linked, const Vector& from, const Vector& exitPos)`
- L151 `std::vector<std::pair<int, std::string>> LandmarkRegions(void* area)`
- L152 `std::vector<DoorName> NumberedDoors()`
- L153 `void NumberDoorExit(ExitMark& e, const std::vector<DoorName>& doors)`
- L154 `std::string Destination(void* area, int target, const std::vector<std::pair<int, std::string>>& landmarks, const std::string& exitText)`
- L155 `bool NearAny(const Vector& p, const std::vector<Vector>& points, float maxM)`
- L156 `std::string Located(const std::string& subject, const Vector& pos)`
- L159 `std::vector<Vector> LockedDoorPositions(void* area)`: exit_memory_doors.cpp
- L160 `void* ClosedDoorNear(void* area, const Vector& p)`
- L161 `void* DoorNear(void* area, const Vector& p)`
- L162 `void* TransitionDoorNear(void* area, const Vector& p, float maxM, const std::string& destName, Vector* outPos)`
- L165 `bool BuildGraph(void* area, const Vector& player, Graph& g)`: exit_memory_graph.cpp
- L166 `std::string ExitToward(const Graph& g, int hop, Vector* outPos)`
- L169 `int IndexAt(const Graph& g, const Vector& pos)`: The graph index of the region `pos` lies in; for a spot off the walkable floor, of the region nearest to it.
- L173 `int TargetIndex(const Graph& g, void* obj, const Vector& pos)`: The graph index of the region to stand in to act on `obj` at `pos`: for a door the side the player walks up to, for anything else the region it is in.
- L174 `Route RouteToIndex(const Graph& g, int t, const Vector& targetPos)`
- L175 `Route RouteTo(const Graph& g, const Vector& targetPos)`
- L176 `std::string RouteText(const Route& r)`
- L177 `bool DoorSides(const Graph& g, const Vector& doorPos, int& nearId, int& farId)`
- L182 `Vector RegionWalkPoint(void* area, int id, const Vector& centre)`: Where a walk into region `id` ends: its centre, or, where no creature can stand there (off the walkable floor or hard against an edge of it), the region's nav node nearest to the centre.
- L183 `std::string DoorBeyond(const Graph& g, const Vector& doorPos, int* outNear)`
- L186 `std::string BehindDoorText(void* area, int farId)`: exit_memory_annotate.cpp
- L189 `std::string TransitionDoorMemory(const Vector& doorPos)`: exit_memory_area_doors.cpp
- L190 `void TickAreaMemory()`
- L193 `void FlushReminder()`: exit_memory_overview.cpp
- L196 `void TickExploreChain()`: exit_memory_explore.cpp

Also 22 constants or variables at namespace scope.

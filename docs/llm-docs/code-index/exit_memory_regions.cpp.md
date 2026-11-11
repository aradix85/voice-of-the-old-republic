# exit_memory_regions.cpp (193 lines)

Source: `patches/Accessibility/exit_memory_regions.cpp`

Regions for the exit-memory modules: the region at a point, the region behind an exit, linked regions, discovered map notes per region, door numbering as the cycles say it, and what lies behind an exit.

## Declarations (in source order)

- L26 `int RegionAt(void* area, const Vector& p)`
- L40 `std::set<int> LinkedRegions(void* area, int self)`: Regions sharing a nav edge with `self` (room_topology's region links).
- L51 `int RegionBehind(void* area, int self, const std::set<int>& linked, const Vector& from, const Vector& exitPos)`
- L73 `std::vector<std::pair<int, std::string>> LandmarkRegions(void* area)`: Discovered landmarks (map notes the player has met) with the region each one sits in.
- L94 `std::vector<DoorName> NumberedDoors()`
- L118 `void NumberDoorExit(ExitMark& e, const std::vector<DoorName>& doors)`
- L129 `std::string Destination(void* area, int target, const std::vector<std::pair<int, std::string>>& landmarks, const std::string& exitText)`
- L164 `bool NearAny(const Vector& p, const std::vector<Vector>& points, float maxM)`
- L171 `std::string Located(const std::string& subject, const Vector& pos)`

# bridge_area.cpp (194 lines)

Source: `patches/Accessibility/bridge_area.cpp`

Local bridge: what is in the area, for /nearby, /overview, /objects and /rooms.

## Declarations (in source order)

- L7 `std::string BuildNearby(float radius)`
- L48 `std::string BuildOverview()`
- L90 `std::string BuildObjects()`: Every door, loot container and ground item in the area with its live state, heard before or not, for the map tool (kotor_kaart.py --live) to lay over the static game files: which doors are open, which containers are e...
- L162 `std::string BuildRooms()`: The area's regions (the mod's room graph): kind, visited, reachable now, and which regions touch.

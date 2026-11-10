# bridge_area.cpp (194 lines)

Source: `patches/Accessibility/bridge_area.cpp`

Local bridge: what is in the area, for /nearby, /overview, /objects and /rooms.

## Declarations (in source order)

- L25 `const char* Bool(bool b)`
- L29 `std::string ObjectFields(C cat, void* obj, const Vector& p)`: "category", "name" (as the mod speaks it), "handle", "x" and "y" of one object, without braces.
- L40 `std::string StateFields(C cat, void* obj)`: The live state /objects adds per category.
- L58 `std::string BuildNearby(const Params& params)`
- L82 `std::string BuildOverview()`
- L120 `std::string BuildObjects()`: Every door, loot container, ground item and creature in the area with its live state and tag, heard before or not, for the map tool (kotor_kaart.py --live) to lay over the static game files: which doors are open, whic...
- L165 `std::string BuildRooms()`: The area's regions (the mod's room graph): kind, visited, reachable now, and which regions touch.

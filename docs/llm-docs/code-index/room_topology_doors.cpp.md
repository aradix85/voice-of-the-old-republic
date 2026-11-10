# room_topology_doors.cpp (410 lines)

Source: `patches/Accessibility/room_topology_doors.cpp`

Doors and area transitions in the region graph: the door and transition snapshots (with the retry until the door list is stable), map notes attached to their doors, the door on a nav edge or near a point, and the classification of an edge as clear, door or blocked.

## Declarations (in source order)

- L29 `const char* DestOrNone(const DoorRecord& door)`
- L36 `void ReadLocString(void* obj, size_t offset, char (&out)[N])`
- L43 `int NearestDoor(const Vector& p, float& outSq)`: The door nearest to p in the horizontal plane, -1 without doors; outSq gets its squared distance.
- L59 `Vector TriggerCrossing(void* trigger, const Vector& origin, int& outVerts)`: Where a transition is crossed: the centroid of the trigger polygon.
- L80 `void SnapshotDoors(void* area)`: Fills g_graph.doors with the area's real doors.
- L133 `void SnapshotTransitionTriggers(void* area)`: Fills g_graph.transitions with the walk-through area transitions, each bound to its nearest nav node; BuildForArea has filled the nodes by now.
- L201 `int FindDoorOnEdge(const Vector& a, const Vector& b)`: The first door on segment AB, or -1: its projection falls on the segment and it lies within kMaxPerpM of it.
- L228 `EdgeResult ClassifyEdge(void* areaForDiag, const Vector& a, const Vector& b, const char* callerCtx)`: Classifies nav segment AB against the walls and the doors.
- L307 `void AttachLandmarksToDoors(void* area)`: Gives each map note to the nearest door within kLandmarkDoorMatchMaxM: the door gets the note's name, and the note is marked claimed so the proximity announce does not say the same name a second later.
- L376 `void MaybeRefreshDoors(void* area)`

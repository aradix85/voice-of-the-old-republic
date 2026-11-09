# room_topology_doors.cpp (575 lines)

Source: `patches/Accessibility/room_topology_doors.cpp`

Doors and area transitions in the region graph: the door and transition snapshots (with the retry until the door list is stable), map notes attached to their doors, the door on a nav edge or near a point, and the classification of an edge as clear, door or blocked.

## Declarations (in source order)

- L47 `int FindDoorNearPoint(const Vector& p, float maxDistM)`: Find the door nearest to `p` within `maxDistM`.
- L75 `void SnapshotDoors(void* area)`: Collect every CSWSDoor in the area into g_graph.doors.
- L181 `void SnapshotTransitionTriggers(void* area)`: Collect every walk-through area-transition trigger into g_graph.transitions.
- L276 `int FindDoorOnEdge(const Vector& a, const Vector& b)`: Door-on-edge test.
- L323 `EdgeResult ClassifyEdge(void* areaForDiag, const Vector& a, const Vector& b, const char* callerCtx)`: Classify a nav-graph segment AB against walls + doors.
- L437 `void AttachLandmarksToDoors(void* area)`: Match each landmark waypoint (registered in transitions.cpp's per-area landmark cache during RebuildLandmarkCache) to the nearest door within kLandmarkDoorMatchMaxM.
- L534 `void MaybeRefreshDoors(void* area)`

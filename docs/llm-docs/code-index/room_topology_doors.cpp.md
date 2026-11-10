# room_topology_doors.cpp (566 lines)

Source: `patches/Accessibility/room_topology_doors.cpp`

Doors and area transitions in the region graph: the door and transition snapshots (with the retry until the door list is stable), map notes attached to their doors, the door on a nav edge or near a point, and the classification of an edge as clear, door or blocked.

## Declarations (in source order)

- L47 `int FindDoorNearPoint(const Vector& p, float maxDistM)`: Find the door nearest to `p` within `maxDistM`.
- L75 `void SnapshotDoors(void* area)`: Collect every CSWSDoor in the area into g_graph.doors.
- L175 `void SnapshotTransitionTriggers(void* area)`: Collect every walk-through area-transition trigger into g_graph.transitions.
- L270 `int FindDoorOnEdge(const Vector& a, const Vector& b)`: Door-on-edge test.
- L317 `EdgeResult ClassifyEdge(void* areaForDiag, const Vector& a, const Vector& b, const char* callerCtx)`: Classify a nav-graph segment AB against walls + doors.
- L431 `void AttachLandmarksToDoors(void* area)`: Match each landmark waypoint (registered in transitions.cpp's per-area landmark cache during RebuildLandmarkCache) to the nearest door within kLandmarkDoorMatchMaxM.
- L525 `void MaybeRefreshDoors(void* area)`

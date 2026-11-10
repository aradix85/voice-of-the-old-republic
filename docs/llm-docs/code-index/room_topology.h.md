# room_topology.h (192 lines)

Source: `patches/Accessibility/room_topology.h`

Single source of truth for "spoken label for the region at world position P". Consumed by transitions (player), view_mode (cursor), map_ui_cursor (inverse-projected). One decomposition per area; all three can't disagree about corridor vs junction vs dead-end.

Algorithm: classify per-node by CSR-adjacency degree on the engine's nav graph, then collapse hub clusters and straight corridor chains into perceptual regions. Implementation lives in room_topology.cpp.

Replaced an earlier per-.lyt-room walkmesh-probe classifier — .lyt- rooms over-segment KOTOR into corridor-sized cells with frequent centroid-in-wall failures. The nav graph is what the engine itself trusts for path solving.

## Declarations (in source order)

- L31 `enum Kind`: sig & 0xff = kind.
- L59 `struct ExitMark`: One exit entry of a region label, exactly as it appears in the label text ("North", "Door South-West to X"), and the world point it leads to — what exit_memory annotates and camera_orient (N) turns to.
- L73 `void BuildForArea(void* area)`: Idempotent on same area pointer.
- L79 `void ResetNavGraphRetry()`: Clears the per-area nav-graph retry latch, re-arming BuildForArea for an area it previously gave up on.
- L84 `void MaybeRefreshDoors(void* area)`: Re-snapshots doors until the door set stabilises (initial snapshot can race a partially-populated server-object array).
- L91 `void AttachLandmarksToDoors(void* area)`: Landmark→door matching pass.
- L93 `bool HasGraphForArea(void* area)`
- L95 `void Reset()`
- L137 `bool LookupAt(void* area, const Vector& worldPos, std::string& outLabel, int& outSig, int& outClusterId, bool allowDiagLog = true, bool requireWallReachable = true, bool zTrusted = false, std::vector<ExitMark>* outExits = nullptr)`: outSig packs kind in low byte + kind-specific bits above.
- L147 `struct ClusterInfo`: What transitions.cpp needs to time a cluster's announce without a second label resolution.
- L165 `bool GetClusterInfo(void* area, int clusterId, ClusterInfo& out)`: False (out left at its defaults) on sentinel ids, unknown ids, or no graph.
- L170 `struct ClusterSummary`: One region of the area's graph, for the exploration overview (exit_memory).
- L175 `bool ListClusters(void* area, std::vector<ClusterSummary>& out)`
- L181 `struct ClusterLink`: Two regions joined by at least one nav-graph edge that does not cross a wall.
- L187 `bool GetClusterLinks(void* area, std::vector<ClusterLink>& out)`
- L189 `void DumpGraphToLog()`

Also 2 constants or variables at namespace scope.

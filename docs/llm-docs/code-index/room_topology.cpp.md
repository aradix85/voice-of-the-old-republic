# room_topology.cpp (4486 lines)

Source: `patches/Accessibility/room_topology.cpp`

Wall-topology decomposition — see room_topology.h for goal + algorithm.

EXPERIMENTAL — alternative-direction-calculation-system branch.

Path 3 (2026-05-13) consumes the engine's per-area nav graph (the same path_points / path_connections fields the engine itself uses for AI pathfinding) and classifies each node by its CSR-adjacency degree: - degree 1               → dead end pointing at its single neighbour - degree 2, same room    → corridor (axis defined by both neighbours) - degree 2, cross-room   → transition (doorway between two rooms) - degree ≥ 3             → junction (comma-separated octant list)

Previous revisions (face-graph flood-fill, parallel-pair sweep) all failed empirically on K1's authoring style — see commit history on this branch. The nav graph is what BioWare ships, and the engine itself trusts it for path solving, so we're operating on the same ground truth the level designers were aware of.

## Declarations (in source order)

- L450 `acc::strings::Id OctantFromVector(float dx, float dy)`: 8-way octant classifier.
- L477 `int OctantBit(acc::strings::Id dir)`: Map an Id::Dir* to a stable bit index 0..7 (for dedup masks).
- L494 `acc::strings::Id BitToOctant(int bit)`: Reverse of OctantBit: bit index → direction Id.
- L520 `int Degree(const acc::engine::navgraph::NavGraphSnapshot& g, int node)`: Per-node degree from the snapshot.
- L537 `void ComputeNodeShapeFeatures(const acc::engine::navgraph::NavGraphSnapshot& g, int n, bool* nodeOpen, bool* nodeRoom, float* nodeFloorZ, bool* nodeDeadEnd, float (*nodeAxisExt)[4])`: Per-node shape features from the 8-ray clearance probe.
- L698 `void SnapshotDoors(void* area)`: Collect every CSWSDoor in the area into g_graph.doors.
- L804 `void LogDoorSnapshotDetails(void* area)`: Per-door diagnostic dump.
- L893 `void SnapshotTransitionTriggers(void* area)`: Collect every walk-through area-transition trigger into g_graph.transitions.
- L978 `std::string RenderTransitionExit(int trigIdx, const Vector& centroid)`: Spoken form of an attached transition trigger, e.g.
- L1015 `int FindDoorOnEdge(const Vector& a, const Vector& b)`: Door-on-edge test.
- L1088 `int FindDoorNearPoint(const Vector& p, float maxDistM)`: Find the door nearest to `p` within `maxDistM`.
- L1113 `EdgeResult ClassifyEdge(void* areaForDiag, const Vector& a, const Vector& b, const char* callerCtx)`: Classify a nav-graph segment AB against walls + doors.
- L1203 `void AppendListEntry(std::string& list, const std::string& entry)`: Append `entry` to a comma-separated list string, inserting ", " before it when the list is non-empty.
- L1218 `void NoteExit(const std::string& entry, const Vector& pos)`
- L1222 `void NoteUnspokenExit(const Vector& pos)`
- L1228 `Vector ExitPoint(int doorIdx, const Vector& fallback)`: Where an exit entry leads: the door's own position when it is a door, else `fallback` (the nav point the shape path measured it from).
- L1252 `std::string RenderDoorDirection(int doorIdx, const char* dirWord)`: Render the door-flavoured replacement of a direction word.
- L1337 `std::string RenderCorridorAxis(int bitA, int doorA, bool deadA, int bitB, int doorB, bool deadB, bool rampA, bool rampB, const Vector& posA, const Vector& posB)`: Render a corridor's axis label into outBuf.
- L1447 `int UFFind(int x)`
- L1455 `void UFUnite(int a, int b)`
- L1472 `std::string DirEntry(acc::strings::Id dirId, bool markDeadEnd, bool markRamp)`: Render one direction entry.
- L1511 `bool ComputeCentroidAxis(const Vector& centroid, float floorZ, const float* memberExt, acc::strings::Id& outAxisId)`: Long-axis test for an area cluster: is the space clearly elongated, and along which axis word?
- L1564 `int AxisOctantMask(acc::strings::Id axisId)`: The two opposite octant bits an elongation axis covers, or 0 for a non-axis Id.
- L1636 `const char* FarEndSuffix(int bit, const Vector& exitPos, const float* bboxXY)`: Far-end exit qualifier (see the FarAt* string ids).
- L1685 `bool ExitIsRamp(int nb, float clusterFloorZ)`: Does the exit toward external neighbour `nb` cross a floor-level change?
- L1695 `void ClassifyAsArea(const acc::engine::navgraph::NavGraphSnapshot& g, int n, const Vector& centroid, float centroidFloorZ, const int* externalNbs, const int* externalSrcs, const int* externalDoorIdx, int externalCount, int areaHint, bool isLargeArea, const float* bboxXY, const float* axisExt, const bool* nodeDeadEnd, const std::string& trigEntries,`: Area path: probe-owned merged spaces (open / room / merged big junction).
- L1857 `void ClassifyAsDeadEnd(const acc::engine::navgraph::NavGraphSnapshot& g, int n, const Vector& centroid, float centroidFloorZ, const int* externalNbs, const int* externalDoorIdx, int trigCount, bool gateAgrees, const std::string& trigEntries, std::string& outLabel, int& outKind, int& outSig, bool& outFiltered)`: Exactly one external edge — a dead end.
- L1975 `bool CorridorEndIsDead(const acc::engine::navgraph::NavGraphSnapshot& g, int nb, const Vector& centroid, const bool* nodeDeadEnd)`: Is this corridor end a cul-de-sac worth flagging?
- L1997 `void ClassifyAsCorridor(const acc::engine::navgraph::NavGraphSnapshot& g, int n, const Vector& centroid, float centroidFloorZ, const int* externalNbs, const int* externalDoorIdx, const bool* nodeDeadEnd, const std::string& trigEntries, std::string& outLabel, int& outKind, int& outSig)`: Exactly two external edges — a corridor run, announced by the axis its two ends lie on.
- L2081 `void ClassifyDemotedToDeadEnd(const Vector& centroid, const int octantDoorIdx[8], const int realExitBits[8], const bool octantAllRamp[8], const Vector octantPos[8], const std::string& trigEntries, std::string& outLabel, int& outKind, int& outSig)`: 3+ external edges that the demote gate reduced to ONE walkable octant: speak it as a dead end, not a junction.
- L2140 `void ClassifyDemotedToCorridor(const Vector& centroid, const int octantDoorIdx[8], const int realExitBits[8], const bool octantAllRamp[8], const Vector octantPos[8], const std::string& trigEntries, std::string& outLabel, int& outKind, int& outSig)`: Same gate, TWO walkable octants: speak it as a corridor.
- L2182 `void EmitJunctionLabel(const bool octantHasExit[8], const bool octantAllDeadEnd[8], const int octantDoorIdx[8], const bool octantAllRamp[8], const Vector octantPos[8], const std::string& trigEntries, std::string& outLabel, int& outKind, int& outSig)`: Second pass: emit octants in canonical order (N, NE, E, SE, S, SW, W, NW).
- L2245 `void ClassifyClusterShape(const acc::engine::navgraph::NavGraphSnapshot& g, const Vector& centroid, const int* externalNbs, const int* externalSrcs, const int* externalDoorIdx, int externalCount, int trigCount, const std::string& trigEntries,`
- L2477 `void ClassifyCluster(const acc::engine::navgraph::NavGraphSnapshot& g, const Vector& centroid, const int* externalNbs, const int* externalSrcs, const int* externalDoorIdx, int externalCount, const int* trigIdxs, int trigCount, int areaHint, float centroidFloorZ,`: Classification entry point: pick the shape, then guarantee the cluster ends up with SOME label.
- L2555 `void LogNavWallCrossings(const acc::engine::navgraph::NavGraphSnapshot& g)`: Diagnostic: nav-graph vs wall-cache crossing check.
- L2666 `void LogTopologyMetrics(const acc::engine::navgraph::NavGraphSnapshot& g, int n)`: Diagnostic-only: dump per-node topology metrics so we can pick a principled gate later.
- L2747 `void LogClusterMemberAdjacency(const acc::engine::navgraph::NavGraphSnapshot& g, int n)`: Diagnostic-only: per multi-node cluster, dump member adjacency.
- L2831 `void AttachLandmarksToDoors(void* area)`: Match each landmark waypoint (registered in transitions.cpp's per-area landmark cache during RebuildLandmarkCache) to the nearest door within kLandmarkDoorMatchMaxM.
- L2928 `void Reset()`
- L2943 `bool HasGraphForArea(void* area)`
- L2949 `void ResetNavGraphRetry()`
- L2953 `void MaybeRefreshDoors(void* area)`
- L3042 `void ComputeChainStraightness(const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`: Corridor straightness: a degree-2 node whose two edges point roughly opposite.
- L3095 `void PassCoreMerge(const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`: ===== Pass 1: core merge ===== Form perceptual cores by walking the nav graph once and unioning each clear, door-free edge whose endpoints are the SAME kind of thing.
- L3195 `void PassCornerFold(const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`: ===== Pass 1b: corner fold ===== A sharp bend (degree-2, not chainStraight) is the corner where two corridor segments meet.
- L3270 `void AbsorbAdjacentStragglers(const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`: ===== Pass 2: straggler absorb ===== Fold leftover corner / doorway nodes into the core they belong to.
- L3383 `void AbsorbStragglersByBoundingBox(void* area, const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`: (b) Bounding-box admission for remaining graph-unattached singletons.
- L3572 `void AbsorbOpenEndedPendants(const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`
- L3671 `void PassClassifyClusters(void* area, const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`: ===== Pass 3: per-cluster classification ===== For each cluster root (the smallest node id in its union-find class), compute the centroid, collect external neighbours (dedup by node id), classify via ClassifyCluster, ...
- L3998 `void BuildForArea(void* area)`
- L4204 `void DumpGraphToLog()`
- L4220 `bool GetClusterInfo(void* area, int clusterId, ClusterInfo& out)`
- L4237 `bool ListClusters(void* area, std::vector<ClusterSummary>& out)`
- L4257 `bool GetClusterLinks(void* area, std::vector<ClusterLink>& out)`
- L4264 `bool LookupAt(void* area, const Vector& worldPos, std::string& outLabel, int& outSig, int& outClusterId, bool allowDiagLog, bool requireWallReachable, bool zTrusted, std::vector<ExitMark>* outExits)`

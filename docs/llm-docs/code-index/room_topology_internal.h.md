# room_topology_internal.h (377 lines)

Source: `patches/Accessibility/room_topology_internal.h`

Internal to the room_topology module (room_topology*.cpp); no other file includes it. The public interface is room_topology.h. Here: what the files share.

## Declarations (in source order)

- L121 `struct DoorRecord`: Cached state for the current area.
- L152 `struct TransitionRecord`: Walk-through area-transition triggers (CSWSTrigger with a non-empty transition destination).
- L167 `struct AreaGraph`
- L224 `struct DoorStabilityState`
- L243 `enum EdgeClass`: Unified edge classifier — Clear / Door / Blocked.
- L249 `struct EdgeResult`
- L283 `struct BuildPasses`: The working set BuildForArea's passes hand to each other.
- L321 `void SnapshotDoors(void* area)`: room_topology_doors.cpp
- L322 `void SnapshotTransitionTriggers(void* area)`
- L323 `int FindDoorOnEdge(const Vector& a, const Vector& b)`
- L324 `EdgeResult ClassifyEdge(void* areaForDiag, const Vector& a, const Vector& b, const char* callerCtx)`
- L327 `int UFFind(int x)`: room_topology_merge.cpp
- L328 `void UFUnite(int a, int b)`
- L329 `void ComputeChainStraightness(const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`
- L330 `void PassCoreMerge(const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`
- L331 `void PassCornerFold(const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`
- L334 `void AbsorbAdjacentStragglers(const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`: room_topology_absorb.cpp
- L335 `void AbsorbStragglersByBoundingBox(void* area, const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`
- L336 `void AbsorbOpenEndedPendants(const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`
- L339 `int Degree(const acc::engine::navgraph::NavGraphSnapshot& g, int node)`: room_topology_shape.cpp
- L340 `void ComputeNodeShapeFeatures(const acc::engine::navgraph::NavGraphSnapshot& g, int n, bool* nodeOpen, bool* nodeRoom, float* nodeFloorZ, bool* nodeDeadEnd, float (*nodeAxisExt)[4])`
- L341 `bool ComputeCentroidAxis(const Vector& centroid, float floorZ, const float* memberExt, acc::strings::Id& outAxisId)`
- L342 `int AxisOctantMask(acc::strings::Id axisId)`
- L343 `void PassClassifyClusters(void* area, const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`
- L346 `acc::strings::Id OctantFromVector(float dx, float dy)`: room_topology_words.cpp
- L347 `int OctantBit(acc::strings::Id dir)`
- L348 `acc::strings::Id BitToOctant(int bit)`
- L349 `std::string RenderTransitionExit(int trigIdx, const Vector& centroid)`
- L350 `void AppendListEntry(std::string& list, const std::string& entry)`
- L351 `void NoteExit(const std::string& entry, const Vector& pos)`
- L352 `Vector ExitPoint(int doorIdx, const Vector& fallback)`
- L353 `std::string RenderDoorDirection(int doorIdx, const char* dirWord)`
- L354 `std::string RenderCorridorAxis(int bitA, int doorA, bool deadA, int bitB, int doorB, bool deadB, bool rampA, bool rampB, const Vector& posA, const Vector& posB)`
- L355 `std::string DirEntry(acc::strings::Id dirId, bool markDeadEnd, bool markRamp = false)`
- L356 `const char* FarEndSuffix(int bit, const Vector& exitPos, const float* bboxXY)`
- L357 `bool ExitIsRamp(int nb, float clusterFloorZ)`
- L360 `void ClassifyAsArea(const acc::engine::navgraph::NavGraphSnapshot& g, int n, const Vector& centroid, float centroidFloorZ, const int* externalNbs, const int* externalSrcs, const int* externalDoorIdx, int externalCount, int areaHint, bool isLargeArea, const float* bboxXY, const float* axisExt, const bool* nodeDeadEnd, const std::string& trigEntries, std::string& outLabel, int& outKind, int& outSig)`: room_topology_label_kinds.cpp
- L361 `void ClassifyAsDeadEnd(const acc::engine::navgraph::NavGraphSnapshot& g, int n, const Vector& centroid, float centroidFloorZ, const int* externalNbs, const int* externalDoorIdx, int trigCount, bool gateAgrees, const std::string& trigEntries, std::string& outLabel, int& outKind, int& outSig, bool& outFiltered)`
- L362 `void ClassifyAsCorridor(const acc::engine::navgraph::NavGraphSnapshot& g, int n, const Vector& centroid, float centroidFloorZ, const int* externalNbs, const int* externalDoorIdx, const bool* nodeDeadEnd, const std::string& trigEntries, std::string& outLabel, int& outKind, int& outSig)`
- L363 `void ClassifyDemotedToDeadEnd(const Vector& centroid, const int octantDoorIdx[8], const int realExitBits[8], const bool octantAllRamp[8], const Vector octantPos[8], const std::string& trigEntries, std::string& outLabel, int& outKind, int& outSig)`
- L364 `void ClassifyDemotedToCorridor(const Vector& centroid, const int octantDoorIdx[8], const int realExitBits[8], const bool octantAllRamp[8], const Vector octantPos[8], const std::string& trigEntries, std::string& outLabel, int& outKind, int& outSig)`
- L365 `void EmitJunctionLabel(const bool octantHasExit[8], const bool octantAllDeadEnd[8], const int octantDoorIdx[8], const bool octantAllRamp[8], const Vector octantPos[8], const std::string& trigEntries, std::string& outLabel, int& outKind, int& outSig)`
- L368 `void ClassifyCluster(const acc::engine::navgraph::NavGraphSnapshot& g, const Vector& centroid, const int* externalNbs, const int* externalSrcs, const int* externalDoorIdx, int externalCount, const int* trigIdxs, int trigCount, int areaHint, float centroidFloorZ, bool isLargeArea, const float* bboxXY, const float* axisExt, const bool* nodeDeadEnd, bool terminalDeadEndAgrees, std::string& outLabel, int& outKind, int& outSig, bool& outFiltered)`: room_topology_label.cpp
- L371 `void LogDoorSnapshotDetails(void* area)`: room_topology_diag.cpp
- L372 `void LogNavWallCrossings(const acc::engine::navgraph::NavGraphSnapshot& g)`
- L373 `void LogTopologyMetrics(const acc::engine::navgraph::NavGraphSnapshot& g, int n)`
- L374 `void LogClusterMemberAdjacency(const acc::engine::navgraph::NavGraphSnapshot& g, int n)`

Also 27 constants or variables at namespace scope.

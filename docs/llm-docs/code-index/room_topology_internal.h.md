# room_topology_internal.h (381 lines)

Source: `patches/Accessibility/room_topology_internal.h`

Internal to the room_topology module (room_topology*.cpp); no other file includes it. The public interface is room_topology.h. Here: what the files share.

## Declarations (in source order)

- L121 `struct DoorRecord`: Cached state for the current area.
- L152 `struct TransitionRecord`: Walk-through area-transition triggers (CSWSTrigger with a non-empty transition destination).
- L167 `struct AreaGraph`
- L224 `struct DoorStabilityState`
- L243 `enum EdgeClass`: Unified edge classifier — Clear / Door / Blocked.
- L249 `struct EdgeResult`
- L287 `struct BuildPasses`: The working set BuildForArea's passes hand to each other.
- L325 `void SnapshotDoors(void* area)`: room_topology_doors.cpp
- L326 `void SnapshotTransitionTriggers(void* area)`
- L327 `int FindDoorOnEdge(const Vector& a, const Vector& b, bool* outBeside = nullptr)`
- L328 `EdgeResult ClassifyEdge(void* areaForDiag, const Vector& a, const Vector& b, const char* callerCtx)`
- L331 `int UFFind(int x)`: room_topology_merge.cpp
- L332 `void UFUnite(int a, int b)`
- L333 `void ComputeChainStraightness(const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`
- L334 `void PassCoreMerge(const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`
- L335 `void PassCornerFold(const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`
- L338 `void AbsorbAdjacentStragglers(const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`: room_topology_absorb.cpp
- L339 `void AbsorbStragglersByBoundingBox(void* area, const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`
- L340 `void AbsorbOpenEndedPendants(const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`
- L343 `int Degree(const acc::engine::navgraph::NavGraphSnapshot& g, int node)`: room_topology_shape.cpp
- L344 `void ComputeNodeShapeFeatures(const acc::engine::navgraph::NavGraphSnapshot& g, int n, bool* nodeOpen, bool* nodeRoom, float* nodeFloorZ, bool* nodeDeadEnd, float (*nodeAxisExt)[4])`
- L345 `bool ComputeCentroidAxis(const Vector& centroid, float floorZ, const float* memberExt, acc::strings::Id& outAxisId)`
- L346 `int AxisOctantMask(acc::strings::Id axisId)`
- L347 `void PassClassifyClusters(void* area, const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`
- L350 `acc::strings::Id OctantFromVector(float dx, float dy)`: room_topology_words.cpp
- L351 `int OctantBit(acc::strings::Id dir)`
- L352 `acc::strings::Id BitToOctant(int bit)`
- L353 `std::string RenderTransitionExit(int trigIdx, const Vector& centroid)`
- L354 `void AppendListEntry(std::string& list, const std::string& entry)`
- L355 `void NoteExit(const std::string& entry, const Vector& pos)`
- L356 `Vector ExitPoint(int doorIdx, const Vector& fallback)`
- L357 `std::string RenderDoorDirection(int doorIdx, const char* dirWord)`
- L358 `std::string RenderCorridorAxis(int bitA, int doorA, bool deadA, int bitB, int doorB, bool deadB, bool rampA, bool rampB, const Vector& posA, const Vector& posB)`
- L359 `std::string DirEntry(acc::strings::Id dirId, bool markDeadEnd, bool markRamp = false)`
- L360 `const char* FarEndSuffix(int bit, const Vector& exitPos, const float* bboxXY)`
- L361 `bool ExitIsRamp(int nb, float clusterFloorZ)`
- L364 `void ClassifyAsArea(const acc::engine::navgraph::NavGraphSnapshot& g, int n, const Vector& centroid, float centroidFloorZ, const int* externalNbs, const int* externalSrcs, const int* externalDoorIdx, int externalCount, int areaHint, bool isLargeArea, const float* bboxXY, const float* axisExt, const bool* nodeDeadEnd, const std::string& trigEntries, std::string& outLabel, int& outKind, int& outSig)`: room_topology_label_kinds.cpp
- L365 `void ClassifyAsDeadEnd(const acc::engine::navgraph::NavGraphSnapshot& g, int n, const Vector& centroid, float centroidFloorZ, const int* externalNbs, const int* externalDoorIdx, int trigCount, bool gateAgrees, const std::string& trigEntries, std::string& outLabel, int& outKind, int& outSig, bool& outFiltered)`
- L366 `void ClassifyAsCorridor(const acc::engine::navgraph::NavGraphSnapshot& g, int n, const Vector& centroid, float centroidFloorZ, const int* externalNbs, const int* externalDoorIdx, const bool* nodeDeadEnd, const std::string& trigEntries, std::string& outLabel, int& outKind, int& outSig)`
- L367 `void ClassifyDemotedToDeadEnd(const Vector& centroid, const int octantDoorIdx[8], const int realExitBits[8], const bool octantAllRamp[8], const Vector octantPos[8], const std::string& trigEntries, std::string& outLabel, int& outKind, int& outSig)`
- L368 `void ClassifyDemotedToCorridor(const Vector& centroid, const int octantDoorIdx[8], const int realExitBits[8], const bool octantAllRamp[8], const Vector octantPos[8], const std::string& trigEntries, std::string& outLabel, int& outKind, int& outSig)`
- L369 `void EmitJunctionLabel(const bool octantHasExit[8], const bool octantAllDeadEnd[8], const int octantDoorIdx[8], const bool octantAllRamp[8], const Vector octantPos[8], const std::string& trigEntries, std::string& outLabel, int& outKind, int& outSig)`
- L372 `void ClassifyCluster(const acc::engine::navgraph::NavGraphSnapshot& g, const Vector& centroid, const int* externalNbs, const int* externalSrcs, const int* externalDoorIdx, int externalCount, const int* trigIdxs, int trigCount, int areaHint, float centroidFloorZ, bool isLargeArea, const float* bboxXY, const float* axisExt, const bool* nodeDeadEnd, bool terminalDeadEndAgrees, std::string& outLabel, int& outKind, int& outSig, bool& outFiltered)`: room_topology_label.cpp
- L375 `void LogDoorSnapshotDetails(void* area)`: room_topology_diag.cpp
- L376 `void LogNavWallCrossings(const acc::engine::navgraph::NavGraphSnapshot& g)`
- L377 `void LogTopologyMetrics(const acc::engine::navgraph::NavGraphSnapshot& g, int n)`
- L378 `void LogClusterMemberAdjacency(const acc::engine::navgraph::NavGraphSnapshot& g, int n)`

Also 27 constants or variables at namespace scope.

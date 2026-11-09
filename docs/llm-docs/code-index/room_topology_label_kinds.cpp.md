# room_topology_label_kinds.cpp (574 lines)

Source: `patches/Accessibility/room_topology_label_kinds.cpp`

The label of one region per kind: open area or room, dead end, corridor, the two demotions (to dead end, to corridor) and the junction label.

## Declarations (in source order)

- L26 `bool CorridorEndIsDead(const acc::engine::navgraph::NavGraphSnapshot& g, int nb, const Vector& centroid, const bool* nodeDeadEnd)`: Is this corridor end a cul-de-sac worth flagging?
- L52 `void ClassifyAsArea(const acc::engine::navgraph::NavGraphSnapshot& g, int n, const Vector& centroid, float centroidFloorZ, const int* externalNbs, const int* externalSrcs, const int* externalDoorIdx, int externalCount, int areaHint, bool isLargeArea, const float* bboxXY, const float* axisExt, const bool* nodeDeadEnd, const std::string& trigEntries,`: Area path: probe-owned merged spaces (open / room / merged big junction).
- L214 `void ClassifyAsDeadEnd(const acc::engine::navgraph::NavGraphSnapshot& g, int n, const Vector& centroid, float centroidFloorZ, const int* externalNbs, const int* externalDoorIdx, int trigCount, bool gateAgrees, const std::string& trigEntries, std::string& outLabel, int& outKind, int& outSig, bool& outFiltered)`: Exactly one external edge — a dead end.
- L325 `void ClassifyAsCorridor(const acc::engine::navgraph::NavGraphSnapshot& g, int n, const Vector& centroid, float centroidFloorZ, const int* externalNbs, const int* externalDoorIdx, const bool* nodeDeadEnd, const std::string& trigEntries, std::string& outLabel, int& outKind, int& outSig)`: Exactly two external edges — a corridor run, announced by the axis its two ends lie on.
- L409 `void ClassifyDemotedToDeadEnd(const Vector& centroid, const int octantDoorIdx[8], const int realExitBits[8], const bool octantAllRamp[8], const Vector octantPos[8], const std::string& trigEntries, std::string& outLabel, int& outKind, int& outSig)`: 3+ external edges that the demote gate reduced to ONE walkable octant: speak it as a dead end, not a junction.
- L468 `void ClassifyDemotedToCorridor(const Vector& centroid, const int octantDoorIdx[8], const int realExitBits[8], const bool octantAllRamp[8], const Vector octantPos[8], const std::string& trigEntries, std::string& outLabel, int& outKind, int& outSig)`: Same gate, TWO walkable octants: speak it as a corridor.
- L510 `void EmitJunctionLabel(const bool octantHasExit[8], const bool octantAllDeadEnd[8], const int octantDoorIdx[8], const bool octantAllRamp[8], const Vector octantPos[8], const std::string& trigEntries, std::string& outLabel, int& outKind, int& outSig)`: Second pass: emit octants in canonical order (N, NE, E, SE, S, SW, W, NW).

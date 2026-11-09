# WalkmeshGeometryAnalysis.cs (606 lines)

Source: `tools/kdev/Core/WalkmeshGeometryAnalysis.cs`

## Declarations (in source order)

- L21 `internal struct V2`
- L22 `internal struct Tri`
- L24 `internal class RoomResult`
- L55 `internal record ClusterStats(int CellCount, float MinX, float MinY, float MaxX, float MaxY)`: Cells reachable by 8-way BFS but not 4-cardinal — i.e.
- L57 `internal class AreaResult`
- L79 `public void Aggregate()`
- L110 `internal static class WalkmeshGeometryAnalysis`
- L113 `internal static RoomResult? AnalyseRoom(string path, float cellSize)`
- L277 `private static void BumpEdge(Dictionary<(uint, uint), int> map, uint a, uint b)`
- L283 `private static int AngleBin(V2 a, V2 b, float rotateDeg)`
- L303 `private static float TriArea(Tri t)`
- L309 `private static float MinInteriorAngleDeg(Tri t)`
- L327 `private static float Dist(V2 a, V2 b)`
- L333 `private static bool PointInTri(float px, float py, Tri t)`
- L351 `private static long PackCell(int cx, int cy)`
- L354 `private static void UnpackCell(long key, out int cx, out int cy)`
- L360 `private static float SparseRasterArea(List<Tri> tris, float cell)`
- L366 `private static HashSet<long> SparseRasterise(List<Tri> tris, float cell)`
- L395 `private static HashSet<long> SparseBfs(HashSet<long> cells, int sx, int sy, bool diagonals)`
- L437 `private static void ComputeSmoothness(HashSet<long> cells, float cellSize, RoomResult result)`: For each walkable cell, compute the longest consecutive walkable run starting at that cell in each of 8 directions, then classify by max-cardinal vs max-diagonal: smooth = max-cardinal ≥ 10 cells (≥ 5 m at 0.5 m grid)...
- L573 `private static List<HashSet<long>> ConnectedComponents8(HashSet<long> cells)`: Connected-component labelling on a HashSet of cells using 8-connectivity.

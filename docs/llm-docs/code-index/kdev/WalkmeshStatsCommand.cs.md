# WalkmeshStatsCommand.cs (471 lines)

Source: `tools/kdev/Commands/WalkmeshStatsCommand.cs`

## Declarations (in source order)

- L30 `public static class WalkmeshStatsCommand`: Analyse extracted KOTOR walkmesh (.wok) files to surface the spatial scales the accessibility wall-cue tuner needs answered: edge length distribution, per-edge "corridor width" (perpendicular distance to nearest paral...
- L39 `public static Command Build()`
- L87 `private static int Run(string source, string? moduleFilter, bool detail, bool minOnly, float minNavigable)`
- L175 `private static void PrintArea(AreaSummary a, bool detail)`
- L209 `private static void PrintGlobal(List<float> edges, List<float> corridors, List<AreaSummary> areas)`
- L242 `private struct V3`
- L244 `private record RoomStats(string RoomName, int WalkableFaceCount, int PerimeterEdgeCount,`
- L247 `private class AreaSummary`
- L256 `private static RoomStats ParseAndAnalyse(string path, float minNavigable)`
- L437 `private static void BumpEdgeWithThird(Dictionary<(uint, uint), int> map,`
- L446 `private static float Length2D(V3 a, V3 b)`
- L454 `private static float Min(List<float> xs)`
- L455 `private static float Max(List<float> xs)`
- L457 `private static float MinOrInf(List<float> xs)`
- L460 `private static float Percentile(List<float> xs, float p)`

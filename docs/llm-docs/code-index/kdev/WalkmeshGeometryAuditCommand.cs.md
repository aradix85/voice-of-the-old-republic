# WalkmeshGeometryAuditCommand.cs (400 lines)

Source: `tools/kdev/Commands/WalkmeshGeometryAuditCommand.cs`

## Declarations (in source order)

- L37 `public static class WalkmeshGeometryAuditCommand`: Geometry-shape audit of extracted KOTOR area walkmeshes.
- L41 `public static Command Build()`
- L78 `private static int Run(string source, string? reportPath, string? moduleFilter, float cellSize)`
- L164 `private static void PrintGlobal(AreaResult global, List<AreaResult> areas, float cell)`
- L238 `private static void WriteReport(string path, AreaResult global, List<AreaResult> areas, float cell)`

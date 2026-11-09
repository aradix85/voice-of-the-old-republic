# SigScanCommand.cs (876 lines)

Source: `tools/kdev/Commands/SigScanCommand.cs`

## Declarations (in source order)

- L45 `public static class SigScanCommand`: Rebase our hardcoded engine addresses onto a different build of swkotor.exe.
- L47 `public static Command Build()`
- L89 `internal static int Run(FileInfo? refFile, FileInfo? targetFile, DirectoryInfo? outDir,`
- L255 `private static bool IsIn(PeInfo pe, PeSection sec, uint va)`
- L263 `private static string? CheckAgainstOriginalBytes(EngineAddress a, Signature sig)`
- L288 `private static int Resolve(List<SigRecord> records, string targetPath,`
- L590 `private static void WriteReports(string dest, List<SigRecord> records, string refPath,`
- L663 `private static void EmitK2Artifacts(string dest, List<SigRecord> records,`: KOTOR 2 mode artifacts: the Steam-K2 → GOG rebase table consumed by acc::addr's K2 lookup, and kotor2-gog.hooks.toml.
- L789 `private static void EmitArtifacts(string dest, List<SigRecord> records, string targetPath)`: Emit the two things the patch actually consumes: the C++ rebase table and the per-version hooks file.
- L864 `private sealed class SigRecord(`

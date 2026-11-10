# BuildCommand.cs (699 lines)

Source: `tools/kdev/Commands/BuildCommand.cs`

## Declarations (in source order)

- L13 `public static class BuildCommand`
- L38 `public static Command Build()`
- L62 `internal static int Run()`: Default entry point used by `kdev dev` and any other internal caller: the incremental build without a forced clean.
- L68 `private readonly record struct CompileUnit(`
- L71 `internal static int RunIncremental(bool clean)`
- L251 `private static bool NeedsRecompile(CompileUnit u)`: Recompile a TU if its obj is missing/stale, the source changed, the deps sidecar is missing, or any tracked header is newer than the obj.
- L266 `private static int CompileChanged(`
- L319 `private static void WriteDeps(CompileUnit u, string clStdout, string projectRoot, string upstream)`: Parse cl /showIncludes output into a deps sidecar.
- L340 `private static string WarningLines(string clStdout)`: The warnings of a compile that succeeded.
- L355 `private static string FilterNotes(string s, string projectRoot)`: Declutter a captured compile failure by dropping /showIncludes note lines (any locale).
- L399 `private static string? FindVcvars32()`
- L459 `private static string? FindOnPath(IDictionary<string, string> env, string exeName)`
- L478 `internal static int RunViaBat()`
- L633 `private static int BuildLoader(KdevConfig config)`
- L682 `private static void CopyDirectory(string source, string destination)`

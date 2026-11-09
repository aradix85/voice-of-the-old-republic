# BuildCommand.cs (666 lines)

Source: `tools/kdev/Commands/BuildCommand.cs`

## Declarations (in source order)

- L13 `public static class BuildCommand`
- L33 `public static Command Build()`
- L57 `internal static int Run()`: Default entry point used by `kdev dev` and any other internal caller: the incremental build without a forced clean.
- L63 `private readonly record struct CompileUnit(string Src, string Obj, string Deps)`
- L65 `internal static int RunIncremental(bool clean)`
- L238 `private static bool NeedsRecompile(CompileUnit u)`: Recompile a TU if its obj is missing/stale, the source changed, the deps sidecar is missing, or any tracked header is newer than the obj.
- L253 `private static int CompileChanged(`
- L300 `private static void WriteDeps(CompileUnit u, string clStdout, string projectRoot, string upstream)`: Parse cl /showIncludes output into a deps sidecar.
- L322 `private static string FilterNotes(string s, string projectRoot)`: Declutter a captured compile failure by dropping /showIncludes note lines (any locale).
- L366 `private static string? FindVcvars32()`
- L426 `private static string? FindOnPath(IDictionary<string, string> env, string exeName)`
- L445 `internal static int RunViaBat()`
- L600 `private static int BuildLoader(KdevConfig config)`
- L649 `private static void CopyDirectory(string source, string destination)`

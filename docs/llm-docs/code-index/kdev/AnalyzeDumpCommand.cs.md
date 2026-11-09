# AnalyzeDumpCommand.cs (677 lines)

Source: `tools/kdev/Commands/AnalyzeDumpCommand.cs`

## Declarations (in source order)

- L22 `public static class AnalyzeDumpCommand`: Open a Windows minidump captured by WER (typically under %LOCALAPPDATA%\CrashDumps), extract the exception record + crashing thread register state, walk the EBP frame chain, ESP-scan for return addresses, and resolve ...
- L27 `public static Command Build()`
- L103 `private static int RunModules(string? dumpArg)`
- L146 `private static int RunPeek(string? dumpArg, string vaStr, int len)`
- L199 `private static bool TryParseHex(string s, out uint v)`
- L209 `private static string ExpandDumpDir()`
- L212 `private static FileInfo[] DiscoverDumps()`
- L222 `private static int RunList()`
- L242 `private static int RunAnalyze(string? dumpArg, int depth, int stackBytes)`
- L296 `private static int AnalyzeOne(FileInfo dump, FunctionTable? table, int depth, int stackBytes)`
- L462 `private static void ResolveAndPrint(string reg, uint value, FunctionTable table)`
- L472 `private static string Resolve(FunctionTable? table, uint addr)`
- L475 `private static string ResolveSuffix(FunctionTable? table, uint addr)`
- L482 `private static bool TryReadU32(IDataReader reader, uint addr, out uint value)`
- L491 `private static int ReadAvailable(IDataReader reader, uint addr, byte[] buf)`
- L508 `private static void DumpHex(byte[] buf, int count, uint baseAddr)`
- L536 `private readonly record struct ExceptionRecord(`
- L547 `private static ExceptionRecord? ReadExceptionStream(string path)`
- L626 `private static string ExceptionCodeName(uint code)`
- L637 `private static string AccessViolationKind(ulong rwFlag)`
- L647 `private readonly struct Ctx32`
- L660 `public static Ctx32 From(byte[] buf)`

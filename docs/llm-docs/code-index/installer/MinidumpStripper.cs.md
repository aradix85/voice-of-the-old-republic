# MinidumpStripper.cs (311 lines)

Source: `installer/KotorAccessibilityInstaller/MinidumpStripper.cs`

## Declarations (in source order)

- L39 `public static class MinidumpStripper`: Shrinks a Windows minidump by dropping the captured memory of modules we never inspect during KOTOR triage (Windows + GPU-driver DLLs), while keeping everything our diagnostics actually use: the game executable + our ...
- L41 `public sealed class Stats`
- L87 `private readonly struct Module`
- L96 `public static Stats StripFile(string inputPath, string outputPath)`: Read a dump, strip it, write the result.
- L105 `public static byte[] Strip(byte[] input, Stats stats)`
- L256 `private static bool ShouldKeep(ulong start, List<Module> modules)`
- L271 `private static List<Module> ReadModules(byte[] input, int modRva)`
- L289 `private static string ReadMinidumpString(byte[] input, int rva)`
- L298 `private static string FileName(string path)`
- L305 `private static uint U32(byte[] b, int o)`
- L306 `private static ulong U64(byte[] b, int o)`
- L307 `private static void PutU32(byte[] b, int o, uint v)`
- L308 `private static void PutU64(byte[] b, int o, ulong v)`

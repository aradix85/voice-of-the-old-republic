# DumpTextCommand.cs (308 lines)

Source: `tools/kdev/Commands/DumpTextCommand.cs`

## Declarations (in source order)

- L45 `public static class DumpTextCommand`: Capture the running game's decrypted image from memory.
- L47 `public static Command Build()`
- L72 `internal static int Run(DirectoryInfo? outDir, int? pid)`
- L217 `private static double PrintableRunRatio(byte[] image, int start, int count)`: Fraction of sampled bytes that are among the handful of opcodes real x86 is saturated with (push/pop reg, mov reg-reg, call rel32, int3 padding).
- L236 `private static Process? ResolveProcess(int? pid)`
- L270 `private sealed class ProcessHandle : IDisposable`
- L283 `public bool Read(ulong address, byte[] buffer)`
- L290 `public void Dispose()`
- L296 `private static extern IntPtr OpenProcess(int access, bool inherit, int pid)`
- L300 `private static extern bool ReadProcessMemory(IntPtr process, IntPtr address,`
- L305 `private static extern bool CloseHandle(IntPtr handle)`

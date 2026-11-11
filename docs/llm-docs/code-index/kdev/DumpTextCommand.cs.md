# DumpTextCommand.cs (293 lines)

Source: `tools/kdev/Commands/DumpTextCommand.cs`

## Declarations (in source order)

- L30 `public static class DumpTextCommand`: Capture the running game's decrypted image from memory.
- L32 `public static Command Build()`
- L57 `internal static int Run(DirectoryInfo? outDir, int? pid)`
- L202 `private static double PrintableRunRatio(byte[] image, int start, int count)`: Fraction of sampled bytes that are among the handful of opcodes real x86 is saturated with (push/pop reg, mov reg-reg, call rel32, int3 padding).
- L221 `private static Process? ResolveProcess(int? pid)`
- L255 `private sealed class ProcessHandle : IDisposable`
- L268 `public bool Read(ulong address, byte[] buffer)`
- L275 `public void Dispose()`
- L281 `private static extern IntPtr OpenProcess(int access, bool inherit, int pid)`
- L285 `private static extern bool ReadProcessMemory(IntPtr process, IntPtr address,`
- L290 `private static extern bool CloseHandle(IntPtr handle)`

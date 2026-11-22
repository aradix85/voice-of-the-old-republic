# LaunchCommand.cs (125 lines)

Source: `tools/kdev/Commands/LaunchCommand.cs`

## Declarations (in source order)

- L7 `public static class LaunchCommand`
- L9 `public static Command Build()`
- L27 `internal static int Run(bool monitor, string game = "")`
- L106 `private static void TryPin(int pid, IntPtr mask, string label)`: The IsWindows() guard below looks redundant — both call sites are already inside Run's if (OperatingSystem.IsWindows()) block, and kdev.csproj pins RuntimeIdentifier=win-x64.

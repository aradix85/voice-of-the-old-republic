# GameProcess.cs (73 lines)

Source: `tools/kdev/Core/GameProcess.cs`

## Declarations (in source order)

- L8 `public static class GameProcess`: Shared helpers for inspecting and terminating the running KOTOR process.
- L14 `public sealed record KillSummary(int FoundCount, int KilledCount, int FailedCount)`
- L30 `public static KillSummary KillAll(string processName = ProcessName)`: Terminates every running instance of the given game and waits for them to exit.

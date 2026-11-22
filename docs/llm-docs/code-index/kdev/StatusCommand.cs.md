# StatusCommand.cs (63 lines)

Source: `tools/kdev/Commands/StatusCommand.cs`

## Declarations (in source order)

- L6 `public static class StatusCommand`
- L8 `public static Command Build()`
- L17 `private static string Installed(string? install, string? exe)`: A game that is not installed is a fact, not a problem: only the game a command acts on has to exist (KdevConfig.ResolveTarget).
- L22 `internal static int Run()`

# Config.cs (327 lines)

Source: `tools/kdev/Core/Config.cs`

## Declarations (in source order)

- L9 `public sealed record KdevConfig(`: Project configuration loaded from kdev.toml at the repo root.
- L46 `public sealed record GameTarget(string Install, string Exe, string ProcessName, string Label)`: Which game a command is acting on.
- L59 `public GameTarget? ResolveTarget(string which)`: Resolve a `--game` selection ("k1" / "k2") to its paths.
- L95 `private static string? InstallProblem(GameTarget target)`: Why this game cannot be used on this machine, or null when it is installed.
- L105 `public static System.CommandLine.Option<string> GameOption()`: The value of the `--game` option every game-facing command takes.
- L115 `public static string? FindConfigFile(string? startDir = null)`: Walks upward from the current directory looking for kdev.toml.
- L131 `public static KdevConfig Load(string? configPath = null)`: Loads and validates kdev.toml.
- L207 `public static KdevConfig? LoadOrPrintErrors(out int exitCode)`: Convenience helper for commands that need a fully-validated config.
- L240 `public IReadOnlyList<string> Validate()`: Returns a list of validation problems (empty if config is fully usable on this machine).
- L266 `private static IReadOnlyList<string> OptionalStringArray(TomlTable table, string dottedKey)`
- L283 `private static string RequireString(TomlTable table, string dottedKey)`
- L293 `private static string? OptionalString(TomlTable table, string dottedKey)`
- L302 `private static object? NavigateTable(TomlTable table, string dottedKey)`
- L314 `private static string ResolvePath(string projectRoot, string path)`
- L322 `public sealed class KdevConfigException : Exception`

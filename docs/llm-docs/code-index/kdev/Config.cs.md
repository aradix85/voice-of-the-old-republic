# Config.cs (299 lines)

Source: `tools/kdev/Core/Config.cs`

## Declarations (in source order)

- L9 `public sealed record KdevConfig(`: Project configuration loaded from kdev.toml at the repo root.
- L46 `public sealed record GameTarget(string Install, string Exe, string ProcessName, string Label)`: Which game a command is acting on.
- L56 `public GameTarget? ResolveTarget(string which)`: Resolve a `--game` selection ("k1" / "k2") to its paths.
- L84 `public static string? FindConfigFile(string? startDir = null)`: Walks upward from the current directory looking for kdev.toml.
- L100 `public static KdevConfig Load(string? configPath = null)`: Loads and validates kdev.toml.
- L176 `public static KdevConfig? LoadOrPrintErrors(out int exitCode)`: Convenience helper for commands that need a fully-validated config.
- L207 `public IReadOnlyList<string> Validate()`: Returns a list of validation problems (empty if config is fully usable on this machine).
- L238 `private static IReadOnlyList<string> OptionalStringArray(TomlTable table, string dottedKey)`
- L255 `private static string RequireString(TomlTable table, string dottedKey)`
- L265 `private static string? OptionalString(TomlTable table, string dottedKey)`
- L274 `private static object? NavigateTable(TomlTable table, string dottedKey)`
- L286 `private static string ResolvePath(string projectRoot, string path)`
- L294 `public sealed class KdevConfigException : Exception`

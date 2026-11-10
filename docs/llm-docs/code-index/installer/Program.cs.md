# Program.cs (601 lines)

Source: `installer/KotorAccessibilityInstaller/Program.cs`

## Declarations (in source order)

- L13 `public enum UpdateChoice`
- L22 `static class Program`
- L26 `static void Main(string[] args)`
- L42 `private static void RunMain(string[] args)`
- L429 `private static string DescribeAllGames()`: One line per game for the maintenance dialogs: installed and at which version, present but not modded, or not found at all.
- L459 `private static string TrimVersion(string version)`: Drop trailing ".0" groups so 0.6.4.0 reads as 0.6.4.
- L467 `private static bool IsModInstalledAt(string gamePath)`: Our runtime DLL sitting in the game folder is what "installed" means.
- L474 `private static GameTarget FirstInstalledTarget()`: First game (KOTOR 1 before KOTOR 2) that has the mod installed, or null when neither does.
- L490 `private sealed class InstalledGame`: One installed copy of the mod: which game, where, and at which version.
- L514 `private static List<InstalledGame> FindInstalledGames(GameTarget only, string pathOverride)`: Every game that currently has the mod, KOTOR 1 first.
- L536 `private static string InstalledPathOf(List<InstalledGame> games, GameTarget target)`: That game's folder among , or null.
- L549 `private static string JoinGameNames(List<InstalledGame> games)`: "KOTOR 1" or "KOTOR 1, KOTOR 2" — for the headings and log lines that have to name a set rather than one game.
- L564 `internal static bool IsNewerVersion(string latestVersion, string installedVersion)`: Welcome → Base-components info → Optional-mods checkboxes → Main install.
- L580 `internal static Version NormalizeVersion(string version)`

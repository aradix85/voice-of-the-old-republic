# K1cpInstaller.cs (295 lines)

Source: `installer/KotorAccessibilityInstaller/ModInstallers/K1cpInstaller.cs`

## Declarations (in source order)

- L32 `public sealed class K1cpInstaller : IModInstaller`: Installs KOTOR 1 Community Patch (K1CP) from the GitHub repo at the pinned commit SHA in .
- L37 `public bool IsSelected(ModSelection selection)`
- L39 `public async Task<ModInstallResult> InstallAsync(ModInstallContext ctx)`
- L126 `private static void ApplyLocaleOverlay(string tslpatchdataDir, GameLocale locale)`
- L217 `private static int NormalizeOverrideLineEndings(string gameDir)`: Convert LF-only `.lyt` / `.vis` files in the game's Override dir to CRLF in-place.
- L252 `private static bool NormalizeFileIfLfOnly(string path)`

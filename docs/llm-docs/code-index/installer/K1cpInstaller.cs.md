# K1cpInstaller.cs (220 lines)

Source: `installer/KotorAccessibilityInstaller/ModInstallers/K1cpInstaller.cs`

## Declarations (in source order)

- L27 `public sealed class K1cpInstaller : IModInstaller`: Installs KOTOR 1 Community Patch (K1CP) from the GitHub repo at the pinned commit SHA in .
- L32 `public bool IsSelected(ModSelection selection)`
- L34 `public async Task<ModInstallResult> InstallAsync(ModInstallContext ctx)`
- L142 `private static int NormalizeOverrideLineEndings(string gameDir)`: Convert LF-only `.lyt` / `.vis` files in the game's Override dir to CRLF in-place.
- L177 `private static bool NormalizeFileIfLfOnly(string path)`

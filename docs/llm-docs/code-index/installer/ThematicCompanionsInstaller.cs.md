# ThematicCompanionsInstaller.cs (203 lines)

Source: `installer/KotorAccessibilityInstaller/ModInstallers/ThematicCompanionsInstaller.cs`

## Declarations (in source order)

- L42 `public sealed class ThematicCompanionsInstaller : IModInstaller`: Installs Thematic Companions (Sniggles &amp; JCarter426) — one sibling mod per game, driven by the same class because they differ only in their pin and their install-option index.
- L48 `public static ThematicCompanionsInstaller ForKotor1()`
- L49 `public static ThematicCompanionsInstaller ForKotor2()`
- L72 `public bool IsSelected(ModSelection selection)`
- L76 `public async Task<ModInstallResult> InstallAsync(ModInstallContext ctx)`

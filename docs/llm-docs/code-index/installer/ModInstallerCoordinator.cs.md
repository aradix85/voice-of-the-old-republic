# ModInstallerCoordinator.cs (147 lines)

Source: `installer/KotorAccessibilityInstaller/ModInstallers/ModInstallerCoordinator.cs`

## Declarations (in source order)

- L18 `public static class ModInstallerCoordinator`: Runs the selected mod installers in a fixed order: K1CP first (it edits dialog.tlk and other archives that subsequent mods build on top of), then the cut-content / mechanic mods, then exe-modifying mods (widescreen) l...
- L20 `public static IReadOnlyList<IModInstaller> BuildPipeline()`
- L56 `public static IReadOnlyList<IModInstaller> BuildKotor2Pipeline()`: Pipeline for a KOTOR 2 install directory, run by AFTER the TSLRCM step: the caller gates on TSLRCM presence (installed this run or detected) so TSLRCM is always first.
- L71 `public static async Task<List<ModInstallResult>> InstallSelectedAsync(`: Run every installer whose returns true.

# TweakPackInstaller.cs (316 lines)

Source: `installer/KotorAccessibilityInstaller/ModInstallers/TweakPackInstaller.cs`

## Declarations (in source order)

- L31 `public sealed class TweakPackInstaller : IModInstaller`: Installs the Unofficial TSLRCM Tweak Pack (Pavijan357, KOTOR 2) — reverts a handful of TSLRCM restorations that Obsidian most likely cut on purpose.
- L36 `public bool IsSelected(ModSelection selection)`
- L47 `private static readonly (string DataPath, string IniName, string Name)[] Components =`: (DataPath relative to the tslpatchdata dir, ini name, display name).
- L58 `public async Task<ModInstallResult> InstallAsync(ModInstallContext ctx)`
- L272 `private static async Task<(bool Success, string Error)> ExtractRarAsync(string rarPath, string destDir)`: Extracts a RAR5 archive with Windows' built-in tar.exe (libarchive, present since Windows 10 1803 and reads RAR5).
- L306 `private static void CopyDirectory(string sourceDir, string destDir)`

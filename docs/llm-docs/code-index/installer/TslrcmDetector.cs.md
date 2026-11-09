# TslrcmDetector.cs (138 lines)

Source: `installer/KotorAccessibilityInstaller/TslrcmDetector.cs`

## Declarations (in source order)

- L40 `public static class TslrcmDetector`: Detects an existing TSLRCM install via the uninstall entry its Inno Setup installer registers (DisplayName "The Sith Lords Restored Content Mod ...").
- L56 `private static bool Matches(string value)`
- L60 `public static bool IsInstalled()`
- L120 `public static int CountPatchedModules(string gameDir)`: How many .mod files sit in the game's Modules folder.

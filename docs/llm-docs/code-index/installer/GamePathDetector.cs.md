# GamePathDetector.cs (213 lines)

Source: `installer/KotorAccessibilityInstaller/GamePathDetector.cs`

## Declarations (in source order)

- L19 `static class GamePathDetector`: Game-install discovery and validation for KOTOR 1 and 2.
- L37 `internal static bool IsRunningAsAdmin()`
- L54 `internal static GameTarget RunningGame()`: The first target whose process is live, or null when neither is.
- L73 `public static string Detect(GameTarget target)`: Locate 's install.
- L106 `private static string SteamLibraryFolderName(GameTarget target)`: Folder name Steam installs each game under inside a library.
- L126 `private static string TryReadSteamAppInstallPath(GameTarget target)`: Steam's own per-app uninstall key, which carries InstallLocation.
- L151 `private static string TryReadGogInstallPath(GameTarget target)`: GOG's per-product key, written by both GOG Galaxy and the offline installers.
- L169 `public static bool IsValidKotor2GamePath(string path)`
- L171 `public static bool IsValidGamePath(string path)`
- L173 `public static bool IsValidGamePath(GameTarget target, string path)`
- L191 `public static bool IsSteamPath(GameTarget target, string gamePath)`: True iff is the install path Steam has registered for this game.
- L200 `private static string NormalizePath(string p)`

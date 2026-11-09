# RegistryManager.cs (125 lines)

Source: `installer/KotorAccessibilityInstaller/RegistryManager.cs`

## Declarations (in source order)

- L16 `public static class RegistryManager`: Manages Windows registry entries for Add/Remove Programs.
- L20 `public static void Register(GameTarget target, string installPath, string version, string uninstallerPath = null)`
- L74 `public static void Unregister(GameTarget target)`
- L108 `public static bool IsRegistered(GameTarget target)`
- L110 `public static string GetRegisteredInstallLocation(GameTarget target)`
- L112 `public static string GetRegisteredVersion(GameTarget target)`
- L114 `private static string ReadValue(GameTarget target, string valueName)`

# WindowsVoicesInstaller.cs (153 lines)

Source: `installer/KotorAccessibilityInstaller/WindowsVoicesInstaller.cs`

## Declarations (in source order)

- L40 `internal static class WindowsVoicesInstaller`: Makes the Windows voices the game cannot see available to the mod's announcement and compass voices.
- L50 `public static void Install(Action<string> report)`
- L108 `private static void CopyKey(RegistryKey source, RegistryKey target)`: Values (kinds and unexpanded %variables% kept) and subkeys, recursively.
- L124 `private static bool Backup()`

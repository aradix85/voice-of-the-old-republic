# HoloPatcherRunner.cs (225 lines)

Source: `installer/KotorAccessibilityInstaller/ModInstallers/HoloPatcherRunner.cs`

## Declarations (in source order)

- L17 `internal static class HoloPatcherRunner`: Drives a headless HoloPatcher install of a staged tslpatchdata/ against a game directory, forwarding throttled stdout lines and a heartbeat to the status callback.
- L65 `public static async Task<(bool Success, string Error)> RunAsync(`: Which entry of the mod's namespaces.ini to install, or null for a mod that has no namespaces.ini at all.
- L141 `static bool IsInternalNoise(string line)`: Local function rather than a field: this is about what we show the user, and it belongs next to the code that shows it.

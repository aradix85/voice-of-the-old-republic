# HoloPatcherProvider.cs (136 lines)

Source: `installer/KotorAccessibilityInstaller/ModInstallers/HoloPatcherProvider.cs`

## Declarations (in source order)

- L46 `public static class HoloPatcherProvider`: Provides HoloPatcher, which drives TSLPatcher-style mod installs (K1CP, K2CP, the Tweak Pack) headlessly.
- L69 `public static async Task<string> ProvideAsync(Action<int> progress = null)`: Writes the bundled HoloPatcher.exe to a fresh staging directory and returns its path.
- L95 `private static void ExtractBundledExe(string targetPath)`
- L117 `public static void Cleanup(string holoPatcherExePath)`

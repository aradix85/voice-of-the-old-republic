# SpatialAudioManager.cs (194 lines)

Source: `installer/KotorAccessibilityInstaller/SpatialAudioManager.cs`

## Declarations (in source order)

- L20 `public static class SpatialAudioManager`: Toggle for the bundled dsoal + OpenAL Soft spatial-audio layer.
- L55 `public sealed class Result`
- L66 `public static bool IsEnabled(string gameDir)`: True if dsound.dll is present in the game folder — the dsoal hijack only works when this exists, so it's the authoritative state bit.
- L77 `public static Result Enable(string gameDir)`: Extracts the bundled dsoal binaries into the game folder and sets EAX=1 under [Sound Options] in swkotor.ini.
- L110 `public static Result Disable(string gameDir)`: Deletes the dsoal files from the game folder and sets EAX=0 .
- L161 `private static void SetEaxValue(string gameDir, bool enable)`: Sets EAX=0 or EAX=1 under [Sound Options] of swkotor.ini.
- L171 `private static void Extract(string resourceShortName, string targetPath)`

# SelfUpdater.cs (534 lines)

Source: `installer/KotorAccessibilityInstaller/SelfUpdater.cs`

## Declarations (in source order)

- L12 `public class SelfUpdateInfo`: What a self-update would install.
- L78 `public static class SelfUpdater`: Keeps the installer itself current, before it installs anything.
- L127 `public static void CleanUp(string[] args)`: Housekeeping for every run: note a completed self-update, drop the ".old" EXE a swap left next to us, and drop staged EXEs left in %TEMP% by earlier runs.
- L182 `public static bool ShouldCheck(string[] args, bool uninstallMode, bool autoUpdateMode, bool quietMode)`: Whether a self-update check makes sense for this launch at all.
- L213 `public static SelfUpdateInfo Check(string runningVersionOverride = null)`: Asks GitHub what the newest installer is.
- L284 `internal static void ParseVersionFile(string text, out string version, out string sha, out long size)`: Reads the lines installer/release.ps1 writes: version=0.8.0 sha256=&lt;64 hex&gt; size=87961850 Unknown keys, blank lines and # comments are ignored, so the format can gain a field later without an older installer cho...
- L315 `public static async Task<string> DownloadAsync(SelfUpdateInfo info,`: Downloads the new EXE into %TEMP% and verifies it.
- L341 `private static void VerifyStagedFile(string staged, SelfUpdateInfo info)`: Hash and version resource must both check out before the file is allowed to replace this process.
- L374 `public static void ReplaceAndLaunch(string staged, string[] args)`: Swaps the verified staged EXE into the running EXE's place and starts it; when the swap cannot be made, starts the staged copy instead.
- L385 `private static bool TryReplaceInPlace(string staged, string target)`: Renames the running EXE aside and moves the staged one into its name.
- L431 `private static void DeleteAsideFile(TimeSpan wait)`: Deletes the ".old" EXE next to us, retrying for up to while the process that left it is still exiting.
- L459 `private static bool IsStagedPath(string path)`
- L473 `private static void Launch(string exe, string[] args)`: Starts the given EXE and hands it this run's arguments, so a launch that carried a game path or a --game keeps carrying it across the restart.
- L493 `private static string Quote(string arg)`
- L496 `private static bool TryDelete(string path)`
- L510 `private static string Sha256OfFile(string path)`
- L522 `private static bool HasSwitch(string[] args, string name)`
- L530 `private static bool IsSwitch(string arg, string name)`

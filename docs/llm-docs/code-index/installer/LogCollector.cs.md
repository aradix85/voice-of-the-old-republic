# LogCollector.cs (863 lines)

Source: `installer/KotorAccessibilityInstaller/LogCollector.cs`

## Declarations (in source order)

- L31 `public static class LogCollector`: Bundles the newest crash dump of EVERY installed game together with the patch log of the session that actually produced it (plus that install's newest log when it is a different session), plus the installer log and a ...
- L33 `public class Result`
- L46 `private sealed class GameSite`: An installed game and where it lives.
- L57 `public static Result Collect(GameTarget target, string gamePath)`: The game the calling flow resolved.
- L249 `public static void RevealInExplorer(string archivePath)`: Open Explorer with the produced archive selected, so the user lands on it ready to attach to an email / Discord message.
- L271 `private static string CompressWith7z(string staging, string downloadsDir, string stamp)`: Compress every file in into a single .7z (LZMA2, max level) in Downloads using the bundled 7zr.exe.
- L351 `private static string CompressWithZip(string staging, string downloadsDir, string stamp)`: Plain Deflate .zip fallback (and the original behaviour) for when the bundled 7zr.exe cannot run on the user's machine.
- L359 `private static void ExtractResource(string resourceShortName, string targetPath)`
- L386 `private static List<GameSite> DiscoverGames(GameTarget preferred, string preferredPath)`: Every game with a real install on this machine.
- L412 `private static string FindNewestPatchLog(string gamePath)`
- L429 `private static DateTime? ParsePatchLogStartUtc(string path)`: Session start encoded in a patch log's file name, as UTC.
- L465 `private static string FindPatchLogForSession(string gamePath, DateTime dumpUtc)`: The patch log for the session that produced the dump written at : the newest log that had already STARTED when the dump landed.
- L493 `private static string FindNewestCrashDump(GameTarget target)`: WER names its dumps &lt;image name&gt;.&lt;pid&gt;.dmp , so the executable name is the game filter.
- L502 `private static string GetDownloadsDir()`
- L512 `private static void WriteSystemInfo(string path, List<GameSite> sites, List<string> dumpPairings)`
- L608 `private static void AppendLoadChain(StringBuilder sb, string gameDir)`: Presence, size and timestamp of every file that has to be there for the mod to enter the game process.
- L656 `private static void AppendVersionMatch(StringBuilder sb, string gameExe, string gameDir)`: Whether the game exe is still the binary the mod was installed against.
- L729 `private static string ReadTargetVersionSha(string configPath)`: Pulls target_version_sha out of patch_config.toml.
- L768 `public static class WerLocalDumps`: Configures Windows Error Reporting to drop a minidump under %LOCALAPPDATA%\CrashDumps when the game crashes.
- L773 `private static string KeyPathFor(GameTarget target)`
- L776 `public static bool IsEnabled()`: True when dump capture is configured for EVERY game.
- L783 `public static bool IsEnabled(GameTarget target)`
- L794 `public static bool Enable()`: Enable dump capture for every game.
- L807 `public static bool Disable(GameTarget target)`: Remove dump capture for one game's executable.
- L826 `public static bool Enable(GameTarget target)`

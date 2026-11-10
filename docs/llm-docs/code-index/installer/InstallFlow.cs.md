# InstallFlow.cs (1079 lines)

Source: `installer/KotorAccessibilityInstaller/InstallFlow.cs`

## Declarations (in source order)

- L21 `static class InstallFlow`: Install-flow orchestration.
- L23 `internal static void RunFullInstallFlow(string gamePath, string pathArgOverride, string latestVersion, string localKpatchPath)`
- L179 `private static string PromptForGamePath(GameTarget target)`: Detection came up empty for a selected game: offer a folder picker before concluding it is absent.
- L241 `internal static void RunKotor2ModFlow(string k2Path)`: KOTOR 2's game-content mods, run before our own install (see the caller for why that order).
- L563 `private static bool RunLocalizedTslrcmInstall(`: TSLRCM step: silent install when the KOTOR 2 folder is known, visible English wizard as fallback (no folder detected, or the silent run failed and the user accepts the fallback).
- L639 `private static TslrcmOutcome RunTslrcmInstall(string k2Path)`
- L707 `private static TslrcmOutcome OfferManualTslrcm(string k2Path, string statusLine, string reason)`: Automatic acquisition of TSLRCM did not work.
- L772 `private static bool ConfirmNoWorkshopContent(string k2Path, string expectedItemId)`: Steam Workshop pre-flight.
- L798 `private static string FindWorkshopContentDir(string k2Path, string expectedItemId)`: &lt;library&gt;/steamapps/workshop/content/208580 if it exists and holds at least one item, else null.
- L838 `private static void RunTslrcmWizardInteractive(string installerExe, string statusLine, bool deleteAfter = true)`: Visible-wizard fallback: announce the handoff (so an unannounced English wizard doesn't steal focus from the screen reader), launch TSLRCM's Inno wizard, block until it exits, delete the temp exe.
- L873 `private static void TryDeleteTempFile(string path)`
- L892 `private static string BackupLocalizedTlk(string k2Path)`: Copy the localized table aside; null when that fails.
- L919 `private static bool RestoreLocalizedTlk(string k2Path, string backupPath)`: Put the localized table back after the English install, keeping TSLRCM's English one as , and remove the trap-kit items whose description StrRefs exist only in that English table (mirroring the FIGS Workshop editions'...
- L964 `private static void RemoveTrapKitOverrides(string k2Path)`: The 7 g_i_trapkit*.uti files the English install drops into Override differ from vanilla by one description StrRef each, re-pointed at corrected text in TSLRCM's English tlk — dangling once the localized table is back.
- L989 `private static bool FilesDiffer(string a, string b)`: Length check first, SHA-256 only on a length tie.
- L1014 `internal static void CollectLogsAndReport(GameTarget target, string gamePath)`: Build a beta-test archive (.7z, LZMA2; .zip fallback) in the user's Downloads folder containing the newest patch log and crash dump of every installed game, the installer log, and a system-info summary.
- L1050 `internal static void ToggleSpatialAudioAndReport(string gamePath)`: Flip dsoal on or off based on its current state, then show a MessageBox so the screen reader announces the new state and reminds the user to restart the game.

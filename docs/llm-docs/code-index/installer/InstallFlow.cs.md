# InstallFlow.cs (712 lines)

Source: `installer/KotorAccessibilityInstaller/InstallFlow.cs`

## Declarations (in source order)

- L21 `static class InstallFlow`: Install-flow orchestration.
- L23 `internal static void RunFullInstallFlow(string gamePath, string pathArgOverride, string latestVersion, string localKpatchPath)`
- L174 `private static string PromptForGamePath(GameTarget target)`: Detection came up empty for a selected game: offer a folder picker before concluding it is absent.
- L236 `internal static void RunKotor2ModFlow(string k2Path)`: KOTOR 2's game-content mods, run before our own install (see the caller for why that order).
- L409 `private static TslrcmOutcome RunTslrcmInstall(string k2Path)`: TSLRCM step: silent install when the KOTOR 2 folder is known, visible wizard as fallback (no folder detected, or the silent run failed and the user accepts the fallback).
- L477 `private static TslrcmOutcome OfferManualTslrcm(string k2Path, string statusLine, string reason)`: Automatic acquisition of TSLRCM did not work.
- L535 `private static bool ConfirmNoWorkshopContent(string k2Path)`: Steam Workshop pre-flight.
- L561 `private static string FindWorkshopContentDir(string k2Path)`: &lt;library&gt;/steamapps/workshop/content/208580 if it exists and holds at least one item, else null.
- L594 `private static void RunTslrcmWizardInteractive(string installerExe, string statusLine, bool deleteAfter = true)`: Visible-wizard fallback: announce the handoff (so an unannounced English wizard doesn't steal focus from the screen reader), launch TSLRCM's Inno wizard, block until it exits, delete the temp exe.
- L629 `private static void TryDeleteTempFile(string path)`
- L647 `internal static void CollectLogsAndReport(GameTarget target, string gamePath)`: Build a beta-test archive (.7z, LZMA2; .zip fallback) in the user's Downloads folder containing the newest patch log and crash dump of every installed game, the installer log, and a system-info summary.
- L683 `internal static void ToggleSpatialAudioAndReport(string gamePath)`: Flip dsoal on or off based on its current state, then show a MessageBox so the screen reader announces the new state and reminds the user to restart the game.

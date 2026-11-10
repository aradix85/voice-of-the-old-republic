# TslrcmInstallForm.cs (540 lines)

Source: `installer/KotorAccessibilityInstaller/TslrcmInstallForm.cs`

## Declarations (in source order)

- L12 `public enum TslrcmOutcome`: Result of a run.
- L68 `public class TslrcmInstallForm : Form`: Downloads the TSLRCM installer exe from DeadlyStream (guest scrape via ), verifies it against the pinned SHA-256 in (the verified fast path — an upstream update that no longer matches is probed structurally and run an...
- L133 `private void InitializeComponents()`
- L203 `private async Task RunAsync()`
- L314 `private async Task RunSilentInstallAsync(string installerExe)`
- L411 `private static bool InnoLogReportsSuccess(string logPath)`: Cheap change-detector for the game's dialog.tlk (size + mtime).
- L426 `private string FingerprintDialogTlk()`
- L446 `private void OnProgress(long done, long total)`: Progress callback from the download stream (worker thread; fires per 64 KB chunk).
- L486 `private void UpdateStatus(string message, bool announce, string logKey = null)`: logKey groups repeating lines onto ONE row in the event log (see EventLogView).
- L508 `private static string ProbeInstallerExe(string path)`: Structural probe for a downloaded file claiming to be the TSLRCM installer: a real PE executable ("MZ" magic) of a size no plausible TSLRCM build could be under.
- L533 `private static void TryDelete(string path)`

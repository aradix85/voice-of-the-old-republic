# TslrcmInstallForm.cs (541 lines)

Source: `installer/KotorAccessibilityInstaller/TslrcmInstallForm.cs`

## Declarations (in source order)

- L12 `public enum TslrcmOutcome`: Result of a run.
- L69 `public class TslrcmInstallForm : Form`: Downloads the TSLRCM installer exe from DeadlyStream (guest scrape via ), verifies it against the pinned SHA-256 in (the verified fast path — an upstream update that no longer matches is probed structurally and run an...
- L134 `private void InitializeComponents()`
- L204 `private async Task RunAsync()`
- L315 `private async Task RunSilentInstallAsync(string installerExe)`
- L412 `private static bool InnoLogReportsSuccess(string logPath)`: Cheap change-detector for the game's dialog.tlk (size + mtime).
- L427 `private string FingerprintDialogTlk()`
- L447 `private void OnProgress(long done, long total)`: Progress callback from the download stream (worker thread; fires per 64 KB chunk).
- L487 `private void UpdateStatus(string message, bool announce, string logKey = null)`: logKey groups repeating lines onto ONE row in the event log (see EventLogView).
- L509 `private static string ProbeInstallerExe(string path)`: Structural probe for a downloaded file claiming to be the TSLRCM installer: a real PE executable ("MZ" magic) of a size no plausible TSLRCM build could be under.
- L534 `private static void TryDelete(string path)`

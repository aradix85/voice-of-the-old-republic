# Kotor2ModsInstallForm.cs (189 lines)

Source: `installer/KotorAccessibilityInstaller/Kotor2ModsInstallForm.cs`

## Declarations (in source order)

- L21 `public class Kotor2ModsInstallForm : Form`: Runs the KOTOR 2 mod pipeline (K2CP → Tweak Pack, see ) against the KOTOR 2 install dir with a progress bar and spoken status updates.
- L50 `private void InitializeComponents()`
- L106 `private async Task RunAsync()`
- L153 `private string AskForManualDownload(ManualDownloadRequest request)`: Host side of : shows the dialog and returns what the user supplied.
- L169 `private void UpdateProgress(int value)`
- L178 `private void UpdateStatus(string message, string logKey = null)`: logKey groups repeating lines onto ONE row in the event log (see EventLogView).

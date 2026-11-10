# MainForm.cs (674 lines)

Source: `installer/KotorAccessibilityInstaller/MainForm.cs`

## Declarations (in source order)

- L14 `public class MainForm : Form`
- L49 `private void InitializeComponents()`
- L180 `private void BrowseButton_Click(object sender, EventArgs e)`
- L201 `private void ValidatePath()`
- L241 `private void RefreshAccessibleDescription()`
- L248 `private async void InstallButton_Click(object sender, EventArgs e)`
- L580 `private void UpdateStatus(string message)`
- L585 `private void UpdateStatus(string message, bool isError, string logKey = null)`: logKey groups repeating lines onto ONE row in the event log (see EventLogView).
- L598 `private void RaiseNotification(string message)`: Why a bare Text write is not enough, and why MostRecent: see ScreenReaderAnnouncer.
- L601 `private void UpdateProgress(int value)`
- L607 `private void SetControlsEnabled(bool enabled)`
- L616 `private static bool AnyOptionalSelected(ModSelection selection)`
- L620 `private void OpenReadme()`
- L634 `private void LaunchGame()`

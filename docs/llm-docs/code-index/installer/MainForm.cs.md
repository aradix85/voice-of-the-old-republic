# MainForm.cs (668 lines)

Source: `installer/KotorAccessibilityInstaller/MainForm.cs`

## Declarations (in source order)

- L14 `public class MainForm : Form`
- L49 `private void InitializeComponents()`
- L180 `private void BrowseButton_Click(object sender, EventArgs e)`
- L201 `private void ValidatePath()`
- L241 `private void RefreshAccessibleDescription()`
- L248 `private async void InstallButton_Click(object sender, EventArgs e)`
- L574 `private void UpdateStatus(string message)`
- L579 `private void UpdateStatus(string message, bool isError, string logKey = null)`: logKey groups repeating lines onto ONE row in the event log (see EventLogView).
- L592 `private void RaiseNotification(string message)`: Why a bare Text write is not enough, and why MostRecent: see ScreenReaderAnnouncer.
- L595 `private void UpdateProgress(int value)`
- L601 `private void SetControlsEnabled(bool enabled)`
- L610 `private static bool AnyOptionalSelected(ModSelection selection)`
- L614 `private void OpenReadme()`
- L628 `private void LaunchGame()`

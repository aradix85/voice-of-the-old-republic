# MainForm.cs (756 lines)

Source: `installer/KotorAccessibilityInstaller/MainForm.cs`

## Declarations (in source order)

- L14 `public class MainForm : Form`
- L59 `protected override void OnShown(EventArgs e)`: In headless mode (--auto-update from the in-game updater), auto-trigger the install on Shown so the user doesn't need to click anything.
- L69 `private void InitializeComponents()`
- L219 `private void BrowseButton_Click(object sender, EventArgs e)`
- L240 `private void ValidatePath()`
- L280 `private void RefreshAccessibleDescription()`
- L287 `private async void InstallButton_Click(object sender, EventArgs e)`
- L658 `private void UpdateStatus(string message)`
- L663 `private void UpdateStatus(string message, bool isError, string logKey = null)`: logKey groups repeating lines onto ONE row in the event log (see EventLogView).
- L676 `private void RaiseNotification(string message)`: Why a bare Text write is not enough, and why MostRecent: see ScreenReaderAnnouncer.
- L679 `private void UpdateProgress(int value)`
- L685 `private void SetControlsEnabled(bool enabled)`
- L695 `private static bool AnyOptionalSelected(ModSelection selection)`
- L699 `private void OpenReadme()`
- L716 `private void LaunchGame()`

# MainForm.cs (730 lines)

Source: `installer/KotorAccessibilityInstaller/MainForm.cs`

## Declarations (in source order)

- L14 `public class MainForm : Form`
- L58 `protected override void OnShown(EventArgs e)`: In headless mode (--auto-update from the in-game updater), auto-trigger the install on Shown so the user doesn't need to click anything.
- L68 `private void InitializeComponents()`
- L207 `private void BrowseButton_Click(object sender, EventArgs e)`
- L228 `private void ValidatePath()`
- L268 `private void RefreshAccessibleDescription()`
- L275 `private async void InstallButton_Click(object sender, EventArgs e)`
- L636 `private void UpdateStatus(string message)`
- L641 `private void UpdateStatus(string message, bool isError, string logKey = null)`: logKey groups repeating lines onto ONE row in the event log (see EventLogView).
- L654 `private void RaiseNotification(string message)`: Why a bare Text write is not enough, and why MostRecent: see ScreenReaderAnnouncer.
- L657 `private void UpdateProgress(int value)`
- L663 `private void SetControlsEnabled(bool enabled)`
- L672 `private static bool AnyOptionalSelected(ModSelection selection)`
- L676 `private void OpenReadme()`
- L690 `private void LaunchGame()`

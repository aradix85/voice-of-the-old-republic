# MainForm.cs (728 lines)

Source: `installer/KotorAccessibilityInstaller/MainForm.cs`

## Declarations (in source order)

- L14 `public class MainForm : Form`
- L56 `protected override void OnShown(EventArgs e)`: In headless mode (--auto-update from the in-game updater), auto-trigger the install on Shown so the user doesn't need to click anything.
- L66 `private void InitializeComponents()`
- L205 `private void BrowseButton_Click(object sender, EventArgs e)`
- L226 `private void ValidatePath()`
- L266 `private void RefreshAccessibleDescription()`
- L273 `private async void InstallButton_Click(object sender, EventArgs e)`
- L634 `private void UpdateStatus(string message)`
- L639 `private void UpdateStatus(string message, bool isError, string logKey = null)`: logKey groups repeating lines onto ONE row in the event log (see EventLogView).
- L652 `private void RaiseNotification(string message)`: Why a bare Text write is not enough, and why MostRecent: see ScreenReaderAnnouncer.
- L655 `private void UpdateProgress(int value)`
- L661 `private void SetControlsEnabled(bool enabled)`
- L670 `private static bool AnyOptionalSelected(ModSelection selection)`
- L674 `private void OpenReadme()`
- L688 `private void LaunchGame()`

# SelfUpdatePromptForm.cs (254 lines)

Source: `installer/KotorAccessibilityInstaller/SelfUpdatePromptForm.cs`

## Declarations (in source order)

- L10 `public enum SelfUpdateChoice`: What happened on the self-update screen.
- L40 `public class SelfUpdatePromptForm : Form`: Offers the installer's own update, and carries it out.
- L63 `private void InitializeComponents()`
- L130 `protected override void OnShown(EventArgs e)`
- L136 `private void OnSkip(object sender, EventArgs e)`
- L153 `private async void OnUpdate(object sender, EventArgs e)`
- L223 `private void SetStatus(string text, bool announce)`
- L230 `protected override void OnFormClosing(FormClosingEventArgs e)`
- L247 `protected override void Dispose(bool disposing)`

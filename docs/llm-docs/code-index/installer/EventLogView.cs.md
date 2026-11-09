# EventLogView.cs (84 lines)

Source: `installer/KotorAccessibilityInstaller/EventLogView.cs`

## Declarations (in source order)

- L43 `public class EventLogView : ListBox`: The running event log shown on the long install screens: what has happened so far, one entry per thing that happened.
- L63 `public void Report(string text, string key = null)`: Appends , or replaces the existing row when has been seen before.

# Logger.cs (169 lines)

Source: `installer/KotorAccessibilityInstaller/Logger.cs`

## Declarations (in source order)

- L25 `public static class Logger`: Buffers installation progress and writes it to the user's Desktop.
- L39 `public static string GetLogPath()`
- L46 `public static string GetPreviousLogPath()`: The run before this one.
- L49 `public static void Info(string message)`
- L50 `public static void Warning(string message)`
- L52 `public static void Error(string message)`
- L58 `public static void Error(string message, Exception ex)`
- L65 `private static void Log(string level, string message)`
- L78 `public static void Flush()`: Write the buffered log, rotating the last one aside first.
- L116 `private static void RotateOnce()`: Move an existing log to .previous.log , once per process.
- L131 `public static bool AskAndSave(bool alwaysAsk = false)`: Write the log unconditionally, then offer to open it when something went wrong.
- L152 `public static void OpenLogFile()`

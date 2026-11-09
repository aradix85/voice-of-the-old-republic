# ModInstallResult.cs (31 lines)

Source: `installer/KotorAccessibilityInstaller/ModInstallers/ModInstallResult.cs`

## Declarations (in source order)

- L9 `public sealed class ModInstallResult`: Outcome of a single run.
- L26 `public static ModInstallResult Ok(string id)`
- L27 `public static ModInstallResult SkippedResult(string id)`
- L28 `public static ModInstallResult Fail(string id, string error)`

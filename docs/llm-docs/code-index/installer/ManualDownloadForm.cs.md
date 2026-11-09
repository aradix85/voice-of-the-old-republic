# ManualDownloadForm.cs (360 lines)

Source: `installer/KotorAccessibilityInstaller/ManualDownloadForm.cs`

## Declarations (in source order)

- L39 `public class ManualDownloadForm : Form`: The fallback that cannot go stale: when we cannot fetch a mod ourselves, the user fetches it and hands it to us, and the install continues automatically from there.
- L57 `public enum FileKind`: What the file should be, so a mis-picked file is caught immediately with a spoken reason rather than failing confusingly several steps later.
- L82 `private void InitializeComponents(string modDisplayName, string reason)`
- L186 `private void OpenDownloadPage()`
- L201 `private void BrowseForFile()`
- L227 `private void Accept(string path)`: Validate shape, report what we know about the file, and close with it if it is usable.
- L269 `private bool HasExpectedMagic(string path)`
- L296 `private string FindLikelyDownload()`: Newest file in Downloads that plausibly IS the mod: the exact expected filename first, then anything with the right extension and the right magic bytes.
- L330 `private static string GetDownloadsDir()`: .NET's SpecialFolder enum has no Downloads entry; same user-profile fallback LogCollector uses.
- L333 `private void AnnounceStatus(string message)`
- L345 `public static string Ask(`: Show the dialog and return the file the user supplied, or null if they skipped.

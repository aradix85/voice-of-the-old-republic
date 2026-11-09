# WorkshopTslrcmForm.cs (673 lines)

Source: `installer/KotorAccessibilityInstaller/WorkshopTslrcmForm.cs`

## Declarations (in source order)

- L70 `public class WorkshopTslrcmForm : Form`: Installs the LOCALIZED TSLRCM for non-English players, by copying the TSLRCM team's official per-language Steam Workshop item into the game folder — taking the player's dialog.tlk from wherever it is actually right, w...
- L75 `public static bool TryGetWorkshopItem(GameLocale locale, out string itemId)`: Official TSLRCM language editions on the Steam Workshop (app 208580), maintained by the TSLRCM team; the German item is author-confirmed at 1.8.6 (2025 comment).
- L99 `public static bool ItemShipsTextTable(GameLocale locale)`: Whether this locale's Workshop item carries its own dialog.tlk .
- L146 `private void InitializeComponents()`
- L232 `private void OpenWorkshopPage(bool reopened)`: Open the item's Workshop page in Steam.
- L272 `private void ReclaimForeground()`: Bring this window back in front of Steam, shortly after Steam has been asked to open a page.
- L299 `private async Task RunAsync()`
- L362 `private bool SteamKnowsWorkshopItem()`: Whether Steam's own workshop bookkeeping mentions this item yet.
- L390 `private bool ItemLooksFullyDownloaded(string itemDir)`: Whether Steam has finished with this item: its folder exists, Steam's manifest lists it, and the bytes on disk match the size the manifest claims.
- L418 `private long DeclaredItemSize()`: The item's size from Steam's manifest, or 0 if not stated.
- L445 `private static int CountFiles(string dir)`
- L460 `private async Task<bool> WaitForItemDownloadAsync(string itemDir)`: Wait until Steam has the item fully on disk.
- L551 `private int CopyItemIntoGame(string itemDir)`: Copy the item's content folders into the game, overwriting.
- L606 `private bool InstallItemTextTable(string itemDir)`: Install the item's own dialog.tlk when it ships one, keeping a backup of the game's.
- L638 `private string ExistingGameFolder(string folderName)`: The install's own spelling of a content folder, or the item's spelling when the install has no such folder yet.
- L658 `private void UpdateStatus(string message, bool announce, string logKey = null)`: logKey groups repeating lines onto ONE row in the event log (see EventLogView).

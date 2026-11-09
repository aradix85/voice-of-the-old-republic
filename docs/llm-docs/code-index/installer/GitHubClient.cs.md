# GitHubClient.cs (536 lines)

Source: `installer/KotorAccessibilityInstaller/GitHubClient.cs`

## Declarations (in source order)

- L16 `public class GitHubClient : IDisposable`: Handles downloading the .kpatch from the mod's GitHub releases.
- L40 `public static string BuildLatestDownloadUrl(string repoUrl, string assetName)`: The ROLLING download URL for an asset on whichever release currently carries the "Latest" badge, e.g.
- L52 `public async Task<string> DownloadTextAsync(string url, TimeSpan timeout)`: Fetches a small text asset.
- L79 `public Task DownloadToPathAsync(string url, string destPath,`: Streams a URL to an explicit destination path with percent progress and cancellation.
- L84 `public async Task<string> GetLatestModVersionAsync(string repoUrl)`
- L105 `public async Task<string> DownloadKPatchAsync(string repoUrl, string assetName, Action<int> progress = null)`
- L114 `public async Task<string> DownloadReleaseAssetAsync(string repoUrl, string assetName, Action<int> progress = null)`: Download a named release asset from the latest release of a GitHub repo.
- L135 `public async Task<string> DownloadReleaseAssetByTagAsync(string repoUrl, string tag, string assetName, Action<int> progress = null)`: Download a named release asset from a specific tagged release.
- L169 `private async Task<string> DownloadTaggedAssetAsync(string repoUrl, string tag, string assetName, Action<int> progress)`: ----- Download path (rate-limit-free primary, API fallback) ------------ Both download entry points funnel here.
- L205 `private async Task DownloadViaApiAssetAsync(`: Outage fallback: resolve the asset's API url from the release JSON and download it with Accept: application/octet-stream (which 302-redirects to the storage backend).
- L245 `private async Task<string> ResolveLatestTagAsync(string repoUrl)`: Resolve a repo's latest release tag.
- L280 `private async Task<string> ResolveLatestTagViaRedirectAsync(string repoUrl)`
- L293 `private async Task<string> ResolveLatestTagViaApiAsync(string repoUrl)`
- L313 `private static string FindAssetApiUrl(string json, string assetName)`: Find the asset whose "name" contains and return its API "url" (api.github.com/.../releases/assets/&lt;id&gt;) — NOT browser_download_url.
- L333 `public sealed class GitBlobEntry`: One entry in a recursive git tree listing — a blob (file) and its path relative to the queried subtree root.
- L350 `public async Task<List<GitBlobEntry>> ListTreeBlobsAsync(`: Lists every blob under in the repo at the given commit.
- L394 `private async Task<string> ResolveSubtreeShaAsync(string owner, string repo, string commitSha, string subdirPath)`
- L426 `public async Task DownloadRawAsync(string url, string destPath, int maxAttempts = 4)`: Downloads a single file from a raw URL with retry on transient (5xx / 429 / network) errors.
- L480 `private static TimeSpan BackoffDelay(int attempt)`
- L483 `private static string Shorten(string sha)`
- L486 `private Task DownloadFileAsync(string url, string destinationPath, Action<int> progress = null,`
- L495 `private async Task DownloadFileAsync(HttpRequestMessage request, string destinationPath,`: Cancellation is honoured per buffer, which is what makes a Cancel button on an 85 MB download mean something: one that only took effect once the transfer had finished would be indistinguishable from one that did nothing.
- L533 `public void Dispose()`

# GitHubClient.cs (391 lines)

Source: `installer/KotorAccessibilityInstaller/GitHubClient.cs`

## Declarations (in source order)

- L16 `public class GitHubClient : IDisposable`: Downloads from GitHub releases: the mod's .kpatch from this fork's bridge-latest release, and pinned third-party assets.
- L32 `public async Task<string> GetModBuildAsync(string repoUrl, string tag)`: Build id of the mod on a release tag: "bridge-" and the first seven characters of the commit the release was built from.
- L65 `public async Task<string> DownloadReleaseAssetByTagAsync(string repoUrl, string tag, string assetName, Action<int> progress = null)`: Download a named release asset from a specific tagged release.
- L99 `private async Task<string> DownloadTaggedAssetAsync(string repoUrl, string tag, string assetName, Action<int> progress)`: ----- Download path (rate-limit-free primary, API fallback) ------------ Release downloads funnel here.
- L135 `private async Task DownloadViaApiAssetAsync(`: Outage fallback: resolve the asset's API url from the release JSON and download it with Accept: application/octet-stream (which 302-redirects to the storage backend).
- L168 `private static string FindAssetApiUrl(string json, string assetName)`: Find the asset whose "name" contains and return its API "url" (api.github.com/.../releases/assets/&lt;id&gt;) — NOT browser_download_url.
- L188 `public sealed class GitBlobEntry`: One entry in a recursive git tree listing — a blob (file) and its path relative to the queried subtree root.
- L205 `public async Task<List<GitBlobEntry>> ListTreeBlobsAsync(`: Lists every blob under in the repo at the given commit.
- L249 `private async Task<string> ResolveSubtreeShaAsync(string owner, string repo, string commitSha, string subdirPath)`
- L281 `public async Task DownloadRawAsync(string url, string destPath, int maxAttempts = 4)`: Downloads a single file from a raw URL with retry on transient (5xx / 429 / network) errors.
- L335 `private static TimeSpan BackoffDelay(int attempt)`
- L338 `private static string Shorten(string sha)`
- L341 `private Task DownloadFileAsync(string url, string destinationPath, Action<int> progress = null,`
- L350 `private async Task DownloadFileAsync(HttpRequestMessage request, string destinationPath,`: Cancellation is honoured per buffer, which is what makes a Cancel button on an 85 MB download mean something: one that only took effect once the transfer had finished would be indistinguishable from one that did nothing.
- L388 `public void Dispose()`

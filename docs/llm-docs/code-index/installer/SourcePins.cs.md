# SourcePins.cs (295 lines)

Source: `installer/KotorAccessibilityInstaller/SourcePins.cs`

## Declarations (in source order)

- L45 `public static class SourcePins`: Where the third-party mod pins come from — hashes, sizes, download pages and pinned commits for TSLRCM, the Tweak Pack, K1CP and K2CP.
- L63 `public sealed class FilePin`
- L72 `public sealed class RepoPin`
- L86 `public sealed class ArchivePin`: A mod that is authored in a git repo but distributed as an archive — K2CP, whose repo carries the recipe and none of the payload.
- L104 `public sealed class ReleasePin`: A mod the author publishes as a GitHub release asset — no scrape, no redistribution, and the tag makes the download reproducible.
- L119 `private sealed class Manifest`
- L166 `public static void Initialize()`: Load the embedded pins, then try to replace them with the repo copy.
- L182 `private static void LoadEmbedded()`
- L218 `private static void TryLoadRemote()`
- L259 `private static Manifest FillGapsFromEmbedded(Manifest remote, Manifest embedded)`: Take the remote manifest, but keep the embedded value for any pin the remote copy does not carry.
- L272 `private static Manifest Parse(string json)`

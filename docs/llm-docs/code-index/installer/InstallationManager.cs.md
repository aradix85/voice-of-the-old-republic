# InstallationManager.cs (864 lines)

Source: `installer/KotorAccessibilityInstaller/InstallationManager.cs`

## Declarations (in source order)

- L17 `public class InstallationManager`: Drives the KOTOR-side installation: stages the bundled KPatchManager runtime (KotorPatcher.dll, sqlite3.dll, AddressDatabases/) into a temp directory, invokes KPatchCore.PatchApplicator against the downloaded .kpatch,...
- L81 `public static long GetEmbeddedResourceLength(string shortName)`: Byte length of a file as it ships inside this installer, or -1 when there is no such resource.
- L268 `public string GetExeFingerprint()`: The short fingerprint of this game's executable, for the unsupported- build message and the bug report that should follow it.
- L302 `public void RevertBundledExePatches()`: Puts the game's executable back the way we found it, undoing the static hooks our bundled engine patches wrote into it (KOTOR 2: 4 GB memory and borderless fullscreen; KOTOR 1 ships nothing that writes to the exe).
- L359 `private static void RevertOneBundledPatch(`: Reverts one bundled patch's static hooks.
- L406 `private static bool IsPatchSupportedForExe(PatchRepository repository, string patchId, string gameExe, out string reason)`: Whether a staged patch declares support for this exact executable.
- L440 `private static bool ManifestDeclaresHash(PatchRepository repository, string patchId, string hash)`: Whether a staged patch's manifest declares this exact SHA-256.
- L461 `public void InstallLoader()`: Drops the dinput8.dll proxy loader into the game root.
- L503 `public void InstallVcRuntime()`: Drops the Visual C++ 2015-2022 runtime (x86) into the game root, so the mod runs against a known-good CRT instead of whatever the machine has in System32.
- L530 `public void InstallPrismRuntime()`: Copies the bundled Prism speech runtime into &lt;game&gt;/patches/.
- L567 `public void InstallOverrideAssets()`: Drops custom WAV samples (and any other Aurora-engine assets we ship) into &lt;game&gt;/Override/.
- L608 `public void InstallPriorityGroup()`: Appends the mod's dedicated full-volume audio priority group to &lt;game&gt;/Override/prioritygroups.2da.
- L637 `public string CopyUninstaller()`: Copies the running installer EXE into the game folder so Add/Remove Programs has a stable uninstaller path even if the user deletes the original download.
- L682 `public static void SweepAbandonedStagingDirs()`: Delete any staging directories left behind, ours or an earlier run's.
- L717 `public void CleanupStaging(string stagingRoot)`: Delete the staging directory, retrying briefly.
- L743 `private static byte[] ReadEmbeddedResourceBytes(string shortName)`
- L758 `private static void ExtractEmbeddedResource(string shortName, string targetPath)`
- L782 `private static void WriteFileResilient(string targetPath, byte[] data, string shortName)`: Writes to , clearing any stale read-only attribute first and retrying on transient I/O / access errors with backoff.
- L811 `private static void ClearReadOnly(string path)`: Best-effort clear of the read-only attribute on an existing target so FileMode.Create can overwrite it.
- L829 `private static string FindResourceName(Assembly assembly, string shortName)`
- L851 `public class LoaderBlockedException : Exception`: Raised when the dinput8.dll proxy loader cannot be written into the game folder even after the resilient writer's retries — almost always because antivirus / Windows Defender blocked the drop (the proxy-DLL name trips...

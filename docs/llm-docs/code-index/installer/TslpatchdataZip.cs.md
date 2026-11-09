# TslpatchdataZip.cs (61 lines)

Source: `installer/KotorAccessibilityInstaller/ModInstallers/TslpatchdataZip.cs`

## Declarations (in source order)

- L20 `internal static class TslpatchdataZip`: Pulls the tslpatchdata/ folder out of a mod's distribution zip.
- L31 `public static void Extract(string zipPath, string destRoot, string logPrefix)`: Extract tslpatchdata/ from into , preserving the folder name (so the result is &lt;destRoot&gt;/tslpatchdata/… ).

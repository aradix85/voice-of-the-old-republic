# InstallerLocale.cs (69 lines)

Source: `installer/KotorAccessibilityInstaller/InstallerLocale.cs`

## Declarations (in source order)

- L14 `public static class InstallerLocale`: The installer's texts: the flat JSON file Locales/en.json, embedded as a resource.
- L18 `public static void Initialize()`
- L24 `public static string Get(string key)`
- L37 `public static string NormalizeLineBreaks(string text)`: Convert the texts' \n line breaks to \r\n .
- L40 `public static string Format(string key, params object[] args)`
- L47 `private static Dictionary<string, string> LoadEmbedded(string resourceName)`

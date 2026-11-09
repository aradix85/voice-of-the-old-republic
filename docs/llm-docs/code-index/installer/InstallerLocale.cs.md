# InstallerLocale.cs (111 lines)

Source: `installer/KotorAccessibilityInstaller/InstallerLocale.cs`

## Declarations (in source order)

- L14 `public static class InstallerLocale`: Static localization class.
- L22 `public static void Initialize(string languageCode)`
- L24 `public static void SetLanguage(string code)`
- L31 `public static string Get(string key)`
- L51 `public static string NormalizeLineBreaks(string text)`: Convert the locale files' \n line breaks to \r\n .
- L54 `public static string Format(string key, params object[] args)`
- L61 `private static void LoadLanguage(string code)`
- L69 `private static Dictionary<string, string> LoadEmbeddedLocale(string code)`

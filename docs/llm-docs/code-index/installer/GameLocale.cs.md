# GameLocale.cs (164 lines)

Source: `installer/KotorAccessibilityInstaller/GameLocale.cs`

## Declarations (in source order)

- L8 `public enum GameLocale`: Game-install locale, sourced from the dialog.tlk header.
- L41 `public static class GameLocaleDetector`: Reads the game install's locale from the dialog.tlk header.
- L55 `public static GameLocale FromInstallerLanguage(string languageCode)`: The game locale corresponding to an installer UI language code ("de", "fr", …), or for one with no counterpart.
- L79 `public static GameLocale Detect(string gameDir)`
- L142 `private static bool LooksCyrillic(FileStream fs, uint blobOffset)`: Samples the tlk's string blob and reports whether it reads as Windows-1251 Cyrillic.

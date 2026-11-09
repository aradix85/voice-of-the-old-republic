# SwkotorIniTweaker.cs (395 lines)

Source: `installer/KotorAccessibilityInstaller/SwkotorIniTweaker.cs`

## Declarations (in source order)

- L31 `public static class SwkotorIniTweaker`: In-place editor for the game's ini that applies the community-recommended stability tweaks alongside our screen-reader compatibility setting, and (on a full install only) the mod's recommended movement keybinds.
- L52 `private static readonly (string Key, string Value)[] KeymapTweaks =`: Movement-keybind defaults for mod users.
- L60 `public sealed class Result`
- L80 `public static Result ApplyAccessibilityDefaults(GameTarget target, string gameDir)`: Apply 's stability + accessibility defaults to its ini.
- L100 `private static Result Combine(Result a, Result b)`
- L117 `public static Result ApplyKeymapDefaults(GameTarget target, string gameDir)`: Apply the mod's movement keybinds (strafe on A/D, camera turn on Y/C) plus the target's (KOTOR 2: SwitchWeaps off H) to the [Keymapping] section of the target's ini.
- L146 `public static Result RemoveKeymapDefaults(GameTarget target, string gameDir)`: Remove the mod's movement keybind lines from [Keymapping] , so the game falls back to its own keymap.2da defaults (strafe on Z/C, camera turn on A/D).
- L160 `private static Result RemoveSectionKeys(`: Delete keys from a section, but only where the value is still the one we wrote — a line the player has since changed is theirs.
- L238 `public static Result ApplyEaxSetting(string gameDir, bool enable)`: Set EAX under [Sound Options] to 1 or 0.
- L248 `private static Result ApplySectionPairs(`: Shared in-place editor: sets each key=value inside of , appending missing keys to the section and the section itself if absent.
- L373 `private static int FindSectionStart(List<string> lines, string sectionHeader)`
- L383 `private static int FindNextSectionStart(List<string> lines, int from)`

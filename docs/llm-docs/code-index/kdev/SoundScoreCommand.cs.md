# SoundScoreCommand.cs (473 lines)

Source: `tools/kdev/Commands/SoundScoreCommand.cs`

## Declarations (in source order)

- L78 `public static class SoundScoreCommand`: Score the extracted WAV corpus for localizability — how well a sound can carry directional information to the ear, especially the hard elevation (up/down) axis used by things like the turret minigame.
- L80 `public static Command Build()`
- L220 `private static int Run(string? inputArg, string? outDirArg, string? reportArg,`
- L314 `private static KdevConfigException NoConfig(string opt)`
- L330 `private static string Tier(int score)`
- L339 `private static int CopyBest(List<SoundScore> results, string outDir, int top, double maxSeconds)`: Copy the best candidates.
- L373 `private static void WriteReport(string path, string input, List<SoundScore> results, int top, int errors)`: Human-readable report.
- L443 `private static void WriteCsv(string path, List<SoundScore> results)`
- L469 `private static string CsvField(string s)`

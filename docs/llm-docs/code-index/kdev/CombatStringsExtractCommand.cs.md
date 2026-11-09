# CombatStringsExtractCommand.cs (893 lines)

Source: `tools/kdev/Commands/CombatStringsExtractCommand.cs`

## Declarations (in source order)

- L73 `public static class CombatStringsExtractCommand`: Extract the combat msg-bus parse anchors for a target locale from that locale's dialog.tlk .
- L75 `public static Command Build()`
- L105 `private static int Run(string? tlkPath, string? langCode, string? outPath)`
- L186 `private static class StrrefMap`: Strref map.
- L258 `private record FieldRule(string Name, string Comment, Func<TlkFile, byte[]?> Extract)`: Field extraction.
- L309 `private static readonly (string Name, string DePlaceholder)[] SpeechLabels = new[]`: Speech-side fields the engine doesn't supply.
- L323 `private static readonly (string Name, string DePlaceholder)[] ShortReplacements = new[]`
- L339 `private record TrailingField(string Name, string Comment, Func<TlkFile, byte[]?>? Engine, string? SpeechDe)`: Results-only labels + effect/save/damage/kill anchors (the rework layer).
- L372 `private static byte[]? TrimAt(byte[]? src, string marker)`
- L382 `private static byte[]? TrimAfter(byte[]? src, string marker)`
- L407 `private static byte[]? LastWordBeforePlaceholder(byte[]? template, string afterPlaceholder, string beforePlaceholder)`: Extract the "noun phrase locator" from a template gap between two placeholders.
- L433 `private static byte[]? TrimLeadingSpaces(byte[]? src)`
- L444 `private static byte[]? StripLeading(byte[]? src, string prefix)`
- L453 `private static byte[]? StripPlaceholders(byte[]? src)`
- L476 `private static byte[]? AppendByte(byte[]? src, byte b)`
- L494 `private static byte[]? SuffixLiteral(TlkFile t, int strref, string? start, string? end, int prefixSpaces = 0)`: Extract a literal substring of a template strref between two placeholder markers.
- L519 `private static byte[]? Between(byte[]? src, string? start, string? end)`
- L545 `private static bool HitMissTagsFilled(TlkFile t)`: True when the hit/miss tag slots carry text (K2 TLKs).
- L558 `private static byte[]? ReconstructHitMissPhrase(TlkFile t, int templateStrref, int verbStrref)`: Reconstruct " VERB mit Angriff auf " by substituting the verb strref into the summary template's CUSTOM1 slot, then extracting the substring between CUSTOM0 and CUSTOM2 (which leaves leading and trailing spaces — the ...
- L588 `private static int IndexOfAscii(byte[] hay, string needle, int from = 0)`
- L602 `private static bool StartsWithAscii(byte[] hay, string needle)`
- L639 `private static byte[]? BetweenPlaceholders(byte[]? s, int n0, int n1)`: Text between placeholder n0 (end) and n1 (start), spacing preserved.
- L653 `private static byte[]? WordBeforePlaceholder(byte[]? s, int n)`: Last whitespace-delimited token before placeholder n (used for the absorb verb that sits immediately before ).
- L668 `private static byte[]? TrimSpaces(byte[]? s)`
- L679 `private static byte[]? Concat(byte[]? a, byte[]? b)`
- L688 `private static byte[] CommonSuffix(byte[] a, byte[] b, byte[] c)`
- L706 `private static byte[]? SaveMarker(TlkFile t)`: save_marker = longest common suffix of the three save-type strings (e.g.
- L719 `private static byte[]? SaveResultPunct(TlkFile t)`: Post-result punctuation (the "!" plus locale spacing) from the save template's gap between the result word and the next placeholder.
- L736 `private static string CppLiteral(byte[]? bytes)`: C++ string-literal emission.
- L781 `private static string EmitSnippet(TlkFile tlk, string langCode, string tlkPath)`: Snippet emission.
- L873 `private static byte[] EncodeAscii(string s)`
- L881 `private static string LanguageIdToCode(int languageId)`

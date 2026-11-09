# EngineAddresses.cs (566 lines)

Source: `tools/kdev/Core/EngineAddresses.cs`

## Declarations (in source order)

- L17 `public sealed record EngineAddress(`: One engine address the patch depends on, and where it came from.
- L63 `public static class EngineAddresses`: Harvests every hardcoded engine address out of the patch sources.
- L134 `public static List<EngineAddress> Collect(string sourceDir, string repoRoot)`: Scan (patches/Accessibility) plus its hooks.toml.
- L238 `public static List<EngineAddress> CollectKotor2(string sourceDir, string repoRoot)`: The KOTOR 2 half of the harvest: the K2-side argument of every addr-layer call (`Pick`/`PickGlobal` second argument, `Kotor2Only` argument) plus every address in kotor2.hooks.toml — hook sites AND consumed_exit_addres...
- L305 `public static Dictionary<uint, uint> CollectHandResolved(string sourceDir)`: The hand-resolved supplement in engine_rebase.cpp 's kXrefTable : reference address to target address, for the few functions sigscan cannot place by their own bytes (identical forwarding thunks, or a near-twin sharing...
- L338 `private static bool TryVa(string hex, out uint va)`: Parse a hex literal and keep it only if it lands in the image's VA window.
- L356 `private static bool TryVaK2(string hex, out uint va)`
- L371 `private static string Snippet(string code)`: A short one-line rendering of the code an unnamed address came from, so the sigscan report says something more useful than "(inline)".
- L383 `private static string StripComments(string line, ref bool inBlockComment)`: Blank out // and /* comment text so quoted addresses in prose are not mistaken for dependencies.
- L409 `private static List<EngineAddress> ParseHooks(string path, string rel)`
- L466 `public sealed record HookParameter(string Source, string Type)`: One `[[hooks.parameters]]` sub-block.
- L475 `public sealed record HookBlock(`: A complete `[[hooks]]` block, every field the patcher consumes.
- L485 `public static class HookFile`
- L493 `public static List<HookBlock> Parse(string path)`: Parse every `[[hooks]]` block of a hooks.toml with full fidelity.
- L563 `private static string Unquote(string s)`

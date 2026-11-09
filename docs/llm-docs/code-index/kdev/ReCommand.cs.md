# ReCommand.cs (520 lines)

Source: `tools/kdev/Commands/ReCommand.cs`

## Declarations (in source order)

- L48 `public static class ReCommand`: Reverse-engineer a whole bounded engine subsystem from Lane's Ghidra DB in one shot — the "decompile-first" playbook, made a command.
- L53 `public static Command Build()`
- L103 `private sealed record FnRec(string Loc, string Ns, string Name, string Sig)`: Records parsed out of the jq NDJSON pass.
- L104 `private sealed record FieldRec(int Offset, string Name, string Type)`
- L105 `private sealed record DtRec(string Name, List<FieldRec> Fields)`
- L107 `private static int Run(string pattern, bool decompile, bool all, int maxFuncs,`
- L301 `private static string RunGhidraDecompile(KdevConfig cfg, List<string> addrs)`: Ghidra headless decompile of a batch of addresses via Decompile.java.
- L356 `private static string CleanGhidraOutput(string raw)`: Strip Ghidra's headless log framing, leaving the decompile.
- L403 `private static bool IsTrivial(FnRec f)`: Classification + slug helpers.
- L417 `private static string Slug(string pattern)`
- L434 `private static string EmitDoc(`: Model-skeleton doc emission.

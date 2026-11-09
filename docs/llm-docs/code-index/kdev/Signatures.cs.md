# Signatures.cs (324 lines)

Source: `tools/kdev/Core/Signatures.cs`

## Declarations (in source order)

- L10 `public sealed record Signature(byte[] Pattern, bool[] Mask, int InstructionCount)`: A byte pattern with per-byte wildcards, anchored at a known address.
- L18 `public override string ToString()`: IDA-style "48 8B ??
- L53 `public static class Signatures`: Builds and resolves relocation-tolerant code signatures.
- L66 `public sealed record BuildResult(Signature? Signature, string? Failure, int MatchesAtFinalLength)`
- L78 `private static bool IsRelocationSensitive(PeInfo pe, uint addr, bool maskAllImageRefs)`: Grow a signature from until it is unique within the reference .text, wildcarding relocation-sensitive operands.
- L93 `public static BuildResult Build(`
- L166 `private static void ApplyWildcards(`: Mark the operand bytes that legitimately differ between builds.
- L209 `private static ulong GetImmediate(in Instruction instr)`
- L223 `private static void Wildcard(List<bool> mask, int offset, int size)`
- L242 `public static Signature? BuildUntilRet(`: A masked pattern covering exactly the instructions from up to and including the first RET — no growth past it.
- L271 `public static bool MatchesAt(`: Masked compare of a pattern against a specific VA in an image.
- L289 `public static int CountMatches(`: Count matches of a masked pattern inside .text, stopping at .

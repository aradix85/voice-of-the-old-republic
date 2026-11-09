# FunctionTable.cs (165 lines)

Source: `tools/kdev/Models/FunctionTable.cs`

## Declarations (in source order)

- L17 `public sealed class FunctionTable`: Address → function-name resolver, built from a Ghidra XML export.
- L19 `public readonly record struct Entry(uint Start, uint End, string Name)`
- L31 `public static FunctionTable LoadFromGhidraXml(string xmlPath)`
- L117 `public Entry? Find(uint address)`: Strict containment lookup: returns the entry whose [Start, End] range contains the address, or null otherwise.
- L131 `public Entry? FindPreceding(uint address)`: Closest preceding function — the entry with the largest Start address &lt;= the query.
- L150 `public string Resolve(uint address)`: Resolve an address to "Name+0xN" — falls back to closest-preceding when not strictly inside a function range, with "+~0xN" to flag the address as outside the recognized body.

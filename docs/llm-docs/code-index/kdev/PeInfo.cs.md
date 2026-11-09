# PeInfo.cs (136 lines)

Source: `tools/kdev/Core/PeInfo.cs`

## Declarations (in source order)

- L22 `public sealed record PeSection(`: Minimal PE32 header reader, shared by kdev dump-text and kdev sigscan .
- L29 `public sealed class PeInfo`
- L44 `public static PeInfo Parse(ReadOnlySpan<byte> buf, ulong? imageBaseOverride = null)`: Parse the headers out of .
- L94 `public PeSection? SectionForRva(uint rva)`
- L104 `public PeSection? SectionNamed(string name)`
- L113 `public int VaToOffset(uint va, bool imageAligned)`: Buffer offset for a virtual address.
- L125 `public uint OffsetToVa(int offset, bool imageAligned)`

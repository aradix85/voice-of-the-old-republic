# TwoDa.cs (155 lines)

Source: `installer/KotorAccessibilityInstaller/TwoDa.cs`

## Declarations (in source order)

- L27 `internal static class TwoDa`: Reads and writes binary 2DA tables (the engine's 2DA V2.b format) for the installer's in-place table patches.
- L31 `internal sealed class Table`
- L40 `internal static Table Parse(byte[] buf)`
- L84 `internal static byte[] Build(Table t)`
- L109 `internal static string Cell(Table t, int row, int colIdx)`
- L117 `internal static int ColumnIndex(Table t, string name)`: Index of the named column (case-insensitive, as the engine matches), or -1.
- L121 `internal static ushort AppendString(Table t, string value)`: Appends to the data block and returns its offset.
- L132 `internal static void SetCell(Table t, int row, int colIdx, string value)`
- L140 `internal static void AddColumnCopy(Table t, string name, int sourceCol)`: Appends a column named whose every cell is the same string as column 's.

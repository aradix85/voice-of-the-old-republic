# SoundTableFixes.cs (112 lines)

Source: `installer/KotorAccessibilityInstaller/SoundTableFixes.cs`

## Declarations (in source order)

- L40 `public static class SoundTableFixes`: Repairs vanilla 2DA data that leaves existing game sounds silent.
- L42 `private sealed record ColumnAlias(string Table, string Misspelled, string Correct)`
- L44 `private sealed record CellFix(string Table, string[] Columns, string Bad, string Good, string What)`
- L66 `public static byte[] Apply(string tableName, byte[] source, List<string> applied)`: Returns with every fix for applied.

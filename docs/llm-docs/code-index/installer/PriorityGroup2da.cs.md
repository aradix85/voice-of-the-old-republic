# PriorityGroup2da.cs (295 lines)

Source: `installer/KotorAccessibilityInstaller/PriorityGroup2da.cs`

## Declarations (in source order)

- L40 `public static class PriorityGroup2da`: Appends the mod's dedicated full-volume audio priority group to prioritygroups.2da .
- L94 `private static readonly (ushort Sentinel, Dictionary<string, string> Row)[] AccRows =`: Every accessibility row we maintain, paired with its sentinel.
- L103 `private sealed class Table`
- L112 `private static Table Parse(byte[] buf)`
- L156 `private static byte[] Build(Table t)`
- L176 `private static string Cell(Table t, int row, int colIdx)`
- L183 `private static int FadeTimeCol(Table t)`
- L187 `private static bool HasSentinel(Table t, int fadeCol, ushort sentinel)`: True if the parsed table carries a row with the given sentinel.
- L197 `public static bool HasAccGroup(byte[] buf)`: True if the table already carries ALL of our sentinel rows.
- L220 `private static string LogicalColumn(string column)`
- L231 `private static void AppendRow(Table t, Dictionary<string, string> row)`: Appends one row (dictionary keyed by logical column name) to the parsed table.
- L271 `public static byte[] AppendAccGroup(byte[] source)`: Returns the 2da bytes with every accessibility row appended that isn't already present.

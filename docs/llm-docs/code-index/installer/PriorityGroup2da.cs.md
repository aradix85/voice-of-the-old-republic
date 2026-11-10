# PriorityGroup2da.cs (265 lines)

Source: `installer/KotorAccessibilityInstaller/PriorityGroup2da.cs`

## Declarations (in source order)

- L40 `public static class PriorityGroup2da`: Appends the mod's dedicated full-volume audio priority group to prioritygroups.2da .
- L66 `private static readonly (ushort Sentinel, Dictionary<string, string> Row)[] AccRows =`: Every accessibility row we maintain, paired with its sentinel.
- L74 `private sealed class Table`
- L83 `private static Table Parse(byte[] buf)`
- L127 `private static byte[] Build(Table t)`
- L147 `private static string Cell(Table t, int row, int colIdx)`
- L154 `private static int FadeTimeCol(Table t)`
- L158 `private static bool HasSentinel(Table t, int fadeCol, ushort sentinel)`: True if the parsed table carries a row with the given sentinel.
- L168 `public static bool HasAccGroup(byte[] buf)`: True if the table already carries ALL of our sentinel rows.
- L191 `private static string LogicalColumn(string column)`
- L202 `private static void AppendRow(Table t, Dictionary<string, string> row)`: Appends one row (dictionary keyed by logical column name) to the parsed table.
- L241 `public static byte[] AppendAccGroup(byte[] source)`: Returns the 2da bytes with every accessibility row appended that isn't already present.

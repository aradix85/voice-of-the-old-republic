# PriorityGroup2da.cs (172 lines)

Source: `installer/KotorAccessibilityInstaller/PriorityGroup2da.cs`

## Declarations (in source order)

- L30 `public static class PriorityGroup2da`: Appends the mod's dedicated full-volume audio priority group to prioritygroups.2da .
- L56 `private static readonly (ushort Sentinel, Dictionary<string, string> Row)[] AccRows =`: Every accessibility row we maintain, paired with its sentinel.
- L62 `private static int FadeTimeCol(TwoDa.Table t)`
- L65 `private static bool HasSentinel(TwoDa.Table t, int fadeCol, ushort sentinel)`: True if the parsed table carries a row with the given sentinel.
- L75 `public static bool HasAccGroup(byte[] buf)`: True if the table already carries ALL of our sentinel rows.
- L98 `private static string LogicalColumn(string column)`
- L109 `private static void AppendRow(TwoDa.Table t, Dictionary<string, string> row)`: Appends one row (dictionary keyed by logical column name) to the parsed table.
- L148 `public static byte[] AppendAccGroup(byte[] source)`: Returns the 2da bytes with every accessibility row appended that isn't already present.

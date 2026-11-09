# StripDumpCommand.cs (58 lines)

Source: `tools/kdev/Commands/StripDumpCommand.cs`

## Declarations (in source order)

- L14 `public static class StripDumpCommand`: Strip a minidump down to the memory our diagnostics use (swkotor.exe + our DLLs, all thread stacks, crash-referenced heap, contexts, module list) and drop stock system/driver module code+data.
- L16 `public static Command Build()`

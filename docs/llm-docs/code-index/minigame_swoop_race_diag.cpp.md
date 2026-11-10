# minigame_swoop_race_diag.cpp (114 lines)

Source: `patches/Accessibility/minigame_swoop_race_diag.cpp`

The swoop race's diagnostic log lines: the minigame struct at entry and a periodic snapshot of the bike.

## Declarations (in source order)

- L36 `void EmitDiagnosticDump(void* miniGame)`
- L68 `void EmitEntrySummary(void* miniGame)`: Brief per-entry summary line.
- L85 `void EmitDiagSnapshot(void* miniGame)`

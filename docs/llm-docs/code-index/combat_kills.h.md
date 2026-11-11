# combat_kills.h (64 lines)

Source: `patches/Accessibility/combat_kills.h`

The kills of one fight, for the summary at its end: the names from the engine's "<actor> killed <target>: N XP" lines, counted per name in the order of the first kill. Engine-free.

## Declarations (in source order)

- L15 `inline std::string KillTarget(const char* line, const char* marker)`: The target of a kill line: what stands between the marker and the last colon.
- L27 `class KillTally`

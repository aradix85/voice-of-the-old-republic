# minigame_swoop_race_internal.h (259 lines)

Source: `patches/Accessibility/minigame_swoop_race_internal.h`

Internal to the minigame_swoop_race module (minigame_swoop_race*.cpp); no other file includes it. The public interface is minigame_swoop_race.h. Here: what the files share.

## Declarations (in source order)

- L147 `struct State`: Module state.
- L242 `void TickRaceTimer(void* miniGame)`: minigame_swoop_race_timer.cpp
- L243 `void SpeakRaceEndMessage()`
- L244 `void FlushRaceEndSpeech(bool force)`
- L247 `void TickGearWatch(void* miniGame)`: minigame_swoop_race_cues.cpp
- L248 `void TickHitPoints(void* miniGame)`
- L249 `void TickStuckHint(void* miniGame)`
- L250 `void TickJumpWatch(void* miniGame)`
- L251 `void TickShiftReady(void* miniGame)`
- L254 `void EmitDiagnosticDump(void* miniGame)`: minigame_swoop_race_diag.cpp
- L255 `void EmitEntrySummary(void* miniGame)`
- L256 `void EmitDiagSnapshot(void* miniGame)`

Also 27 constants or variables at namespace scope.

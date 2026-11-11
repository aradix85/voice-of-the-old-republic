# minigame_swoop_race_internal.h (162 lines)

Source: `patches/Accessibility/minigame_swoop_race_internal.h`

Internal to the minigame_swoop_race module (minigame_swoop_race*.cpp); no other file includes it. The public interface is minigame_swoop_race.h. Here: what the files share.

## Declarations (in source order)

- L67 `struct State`: The module state.
- L145 `void TickRaceTimer(void* miniGame)`: minigame_swoop_race_timer.cpp
- L146 `void SpeakRaceEndMessage()`
- L147 `void FlushRaceEndSpeech(bool force)`
- L150 `void TickGearWatch(void* miniGame)`: minigame_swoop_race_cues.cpp
- L151 `void TickHitPoints(void* miniGame)`
- L152 `void TickStuckHint(void* miniGame)`
- L153 `void TickJumpWatch(void* miniGame)`
- L154 `void TickShiftReady(void* miniGame)`
- L155 `void TickAutoShift()`
- L158 `void EmitEntrySummary(void* miniGame)`: minigame_swoop_race_diag.cpp
- L159 `void EmitDiagSnapshot(void* miniGame)`

Also 13 constants or variables at namespace scope.

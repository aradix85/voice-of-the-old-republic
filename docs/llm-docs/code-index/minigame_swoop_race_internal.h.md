# minigame_swoop_race_internal.h (233 lines)

Source: `patches/Accessibility/minigame_swoop_race_internal.h`

Internal to the minigame_swoop_race module (minigame_swoop_race*.cpp); no other file includes it. The public interface is minigame_swoop_race.h. Here: what the files share.

## Declarations (in source order)

- L132 `struct State`: The module state.
- L217 `void TickRaceTimer(void* miniGame)`: minigame_swoop_race_timer.cpp
- L218 `void SpeakRaceEndMessage()`
- L219 `void FlushRaceEndSpeech(bool force)`
- L222 `void TickGearWatch(void* miniGame)`: minigame_swoop_race_cues.cpp
- L223 `void TickHitPoints(void* miniGame)`
- L224 `void TickStuckHint(void* miniGame)`
- L225 `void TickJumpWatch(void* miniGame)`
- L226 `void TickShiftReady(void* miniGame)`
- L229 `void EmitEntrySummary(void* miniGame)`: minigame_swoop_race_diag.cpp
- L230 `void EmitDiagSnapshot(void* miniGame)`

Also 24 constants or variables at namespace scope.

# minigame_swoop_race_internal.h (172 lines)

Source: `patches/Accessibility/minigame_swoop_race_internal.h`

Internal to the minigame_swoop_race module (minigame_swoop_race*.cpp); no other file includes it. The public interface is minigame_swoop_race.h. Here: what the files share.

## Declarations (in source order)

- L71 `struct State`: The module state.
- L156 `void TickRaceTimer(void* miniGame)`: minigame_swoop_race_timer.cpp
- L157 `void SpeakRaceEndMessage()`
- L158 `void FlushRaceEndSpeech(bool force)`
- L161 `void TickGearWatch(void* miniGame)`: minigame_swoop_race_cues.cpp
- L162 `void TickHitPoints(void* miniGame)`
- L163 `void TickStuckHint(void* miniGame)`
- L164 `void TickJumpWatch(void* miniGame)`
- L165 `void TickShiftReady(void* miniGame)`
- L168 `void EmitEntrySummary(void* miniGame)`: minigame_swoop_race_diag.cpp
- L169 `void EmitDiagSnapshot(void* miniGame)`

Also 13 constants or variables at namespace scope.

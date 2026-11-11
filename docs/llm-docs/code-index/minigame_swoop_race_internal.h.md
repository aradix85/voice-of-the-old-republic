# minigame_swoop_race_internal.h (160 lines)

Source: `patches/Accessibility/minigame_swoop_race_internal.h`

Internal to the minigame_swoop_race module (minigame_swoop_race*.cpp); no other file includes it. The public interface is minigame_swoop_race.h. Here: what the files share.

## Declarations (in source order)

- L67 `struct State`: The module state.
- L144 `void TickRaceTimer(void* miniGame)`: minigame_swoop_race_timer.cpp
- L145 `void SpeakRaceEndMessage()`
- L146 `void FlushRaceEndSpeech(bool force)`
- L149 `void TickGearWatch(void* miniGame)`: minigame_swoop_race_cues.cpp
- L150 `void TickHitPoints(void* miniGame)`
- L151 `void TickStuckHint(void* miniGame)`
- L152 `void TickJumpWatch(void* miniGame)`
- L153 `void TickShiftReady(void* miniGame)`
- L156 `void EmitEntrySummary(void* miniGame)`: minigame_swoop_race_diag.cpp
- L157 `void EmitDiagSnapshot(void* miniGame)`

Also 13 constants or variables at namespace scope.

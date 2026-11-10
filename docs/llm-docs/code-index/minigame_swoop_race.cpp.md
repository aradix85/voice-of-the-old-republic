# minigame_swoop_race.cpp (1344 lines)

Source: `patches/Accessibility/minigame_swoop_race.cpp`

Swoop race minigame accessibility — see swoop_race.h for design.

Phase 1 lay-off (current): entry/exit announce + keybind cheat sheet, best-effort gear-shift announce derived from CSWTrackFollower.speed jumps, per-tick diagnostic snapshot (speed + tunnel offset). Latches the CSWMiniGame pointer on first detect because the area chain (GetCurrentArea → GetClientArea → mini_game) churns during the race-start transition and stops resolving the minigame even while the race is still running — verified live by patch-20260524-163552.log where ENTER fired at T+0 and a spurious EXIT fired at T+125 ms even though the actual race continued for 40 more seconds.

## Declarations (in source order)

- L253 `inline float ShiftGateSpeed(int gear)`
- L427 `void* ReadMiniGameViaArea()`: Read CSWCArea.mini_game via the player-area chain.
- L440 `bool LatchedStillValid()`: Validate a latched CSWMiniGame pointer by re-reading its vtable and comparing against the value captured at latch time.
- L501 `bool ReadWorldClockMs(void* server, uint32_t& outMs, uint32_t& outMph)`: Current world-clock time-of-day in ms (the value GetTime* read) + the calendar's minutes-per-hour.
- L553 `void TickRaceTimer(void* miniGame)`: Per-tick: cache the start stamp once, then freeze the elapsed at each finish-band tick.
- L671 `void EmitDiagnosticDump(void* miniGame)`
- L703 `void EmitEntrySummary(void* miniGame)`: Brief per-entry summary line.
- L720 `void EmitDiagSnapshot(void* miniGame)`
- L754 `void AnnounceGear(int gear, const char* why)`: Speak the gear the bike is now in and re-arm the shift-ready cue for it.
- L767 `void TickGearWatch(void* miniGame)`
- L824 `void TickHitPoints(void* miniGame)`
- L853 `void DecideWreckAtFinish(void* miniGame)`: At the track's own race-end flag: finish or wreck?
- L901 `void TickStuckHint(void* miniGame)`
- L962 `void TickJumpWatch(void* miniGame)`
- L1005 `void TickShiftReady(void* miniGame)`: "You can shift now" cue (Option A — exact onfire.ncs gate, see kShiftGateSpeed).
- L1055 `void AnnounceEntry()`
- L1073 `void SpeakRaceEndMessage()`
- L1119 `void FlushRaceEndSpeech(bool force)`: Speaks the pending race time once its delay has elapsed (force = the race module is exiting: speak whatever is pending now rather than lose it).
- L1132 `void HandleEnter(void* mg)`
- L1180 `void HandleExit()`
- L1206 `bool RunPlayerSlotScript(uintptr_t runner, const char* what)`: Run one of the K2 player bike's slot scripts through the engine's own runner, mirroring the guards the client input handler applies (a live swoop-type minigame with a player).
- L1226 `bool IsActive()`
- L1228 `bool TryShift()`
- L1249 `bool TryJump()`
- L1253 `void Tick()`

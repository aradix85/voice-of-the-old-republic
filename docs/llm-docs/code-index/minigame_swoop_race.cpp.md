# minigame_swoop_race.cpp (1349 lines)

Source: `patches/Accessibility/minigame_swoop_race.cpp`

Swoop race minigame accessibility — see swoop_race.h for design.

Phase 1 lay-off (current): entry/exit announce + keybind cheat sheet, best-effort gear-shift announce derived from CSWTrackFollower.speed jumps, per-tick diagnostic snapshot (speed + tunnel offset). Latches the CSWMiniGame pointer on first detect because the area chain (GetCurrentArea → GetClientArea → mini_game) churns during the race-start transition and stops resolving the minigame even while the race is still running — verified live by patch-20260524-163552.log where ENTER fired at T+0 and a spurious EXIT fired at T+125 ms even though the actual race continued for 40 more seconds.

## Declarations (in source order)

- L256 `inline float ShiftGateSpeed(int gear)`
- L430 `void* ReadMiniGameViaArea()`: Read CSWCArea.mini_game via the player-area chain.
- L443 `bool LatchedStillValid()`: Validate a latched CSWMiniGame pointer by re-reading its vtable and comparing against the value captured at latch time.
- L504 `bool ReadWorldClockMs(void* server, uint32_t& outMs, uint32_t& outMph)`: Current world-clock time-of-day in ms (the value GetTime* read) + the calendar's minutes-per-hour.
- L556 `void TickRaceTimer(void* miniGame)`: Per-tick: cache the start stamp once, then freeze the elapsed at each finish-band tick.
- L674 `void EmitDiagnosticDump(void* miniGame)`
- L706 `void EmitEntrySummary(void* miniGame)`: Brief per-entry summary line.
- L723 `void EmitDiagSnapshot(void* miniGame)`
- L757 `void AnnounceGear(int gear, const char* why)`: Speak the gear the bike is now in and re-arm the shift-ready cue for it.
- L770 `void TickGearWatch(void* miniGame)`
- L827 `void TickHitPoints(void* miniGame)`
- L856 `void DecideWreckAtFinish(void* miniGame)`: At the track's own race-end flag: finish or wreck?
- L904 `void TickStuckHint(void* miniGame)`
- L965 `void TickJumpWatch(void* miniGame)`
- L1008 `void TickShiftReady(void* miniGame)`: "You can shift now" cue (Option A — exact onfire.ncs gate, see kShiftGateSpeed).
- L1059 `void AnnounceEntry()`
- L1078 `void SpeakRaceEndMessage()`
- L1124 `void FlushRaceEndSpeech(bool force)`: Speaks the pending race time once its delay has elapsed (force = the race module is exiting: speak whatever is pending now rather than lose it).
- L1137 `void HandleEnter(void* mg)`
- L1185 `void HandleExit()`
- L1211 `bool RunPlayerSlotScript(uintptr_t runner, const char* what)`: Run one of the K2 player bike's slot scripts through the engine's own runner, mirroring the guards the client input handler applies (a live swoop-type minigame with a player).
- L1231 `bool IsActive()`
- L1233 `bool TryShift()`
- L1254 `bool TryJump()`
- L1258 `void Tick()`

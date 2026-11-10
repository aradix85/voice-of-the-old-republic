# minigame_swoop_race.cpp (1315 lines)

Source: `patches/Accessibility/minigame_swoop_race.cpp`

Swoop race minigame accessibility — see minigame_swoop_race.h for design.

Phase 1 lay-off (current): entry/exit announce + keybind cheat sheet, best-effort gear-shift announce derived from CSWTrackFollower.speed jumps, per-tick diagnostic snapshot (speed + tunnel offset). Latches the CSWMiniGame pointer on first detect because the area chain (GetCurrentArea → GetClientArea → mini_game) churns during the race-start transition and stops resolving the minigame even while the race is still running — verified live by patch-20260524-163552.log where ENTER fired at T+0 and a spurious EXIT fired at T+125 ms even though the actual race continued for 40 more seconds.

## Declarations (in source order)

- L244 `inline float ShiftGateSpeed(int gear)`
- L402 `void* ReadMiniGameViaArea()`: Read CSWCArea.mini_game via the player-area chain.
- L415 `bool LatchedStillValid()`: Validate a latched CSWMiniGame pointer by re-reading its vtable and comparing against the value captured at latch time.
- L476 `bool ReadWorldClockMs(void* server, uint32_t& outMs, uint32_t& outMph)`: Current world-clock time-of-day in ms (the value GetTime* read) + the calendar's minutes-per-hour.
- L528 `void TickRaceTimer(void* miniGame)`: Per-tick: cache the start stamp once, then freeze the elapsed at each finish-band tick.
- L646 `void EmitDiagnosticDump(void* miniGame)`
- L678 `void EmitEntrySummary(void* miniGame)`: Brief per-entry summary line.
- L695 `void EmitDiagSnapshot(void* miniGame)`
- L729 `void AnnounceGear(int gear, const char* why)`: Speak the gear the bike is now in and re-arm the shift-ready cue for it.
- L742 `void TickGearWatch(void* miniGame)`
- L799 `void TickHitPoints(void* miniGame)`
- L828 `void DecideWreckAtFinish(void* miniGame)`: At the track's own race-end flag: finish or wreck?
- L876 `void TickStuckHint(void* miniGame)`
- L937 `void TickJumpWatch(void* miniGame)`
- L980 `void TickShiftReady(void* miniGame)`: "You can shift now" cue (Option A — exact onfire.ncs gate, see kShiftGateSpeed).
- L1026 `void AnnounceEntry()`
- L1044 `void SpeakRaceEndMessage()`
- L1090 `void FlushRaceEndSpeech(bool force)`: Speaks the pending race time once its delay has elapsed (force = the race module is exiting: speak whatever is pending now rather than lose it).
- L1103 `void HandleEnter(void* mg)`
- L1151 `void HandleExit()`
- L1177 `bool RunPlayerSlotScript(uintptr_t runner, const char* what)`: Run one of the K2 player bike's slot scripts through the engine's own runner, mirroring the guards the client input handler applies (a live swoop-type minigame with a player).
- L1197 `bool IsActive()`
- L1199 `bool TryShift()`
- L1220 `bool TryJump()`
- L1224 `void Tick()`

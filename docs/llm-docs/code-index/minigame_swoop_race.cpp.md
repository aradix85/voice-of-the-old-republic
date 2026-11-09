# minigame_swoop_race.cpp (1346 lines)

Source: `patches/Accessibility/minigame_swoop_race.cpp`

Swoop race minigame accessibility — see swoop_race.h for design.

Phase 1 lay-off (current): entry/exit announce + keybind cheat sheet, best-effort gear-shift announce derived from CSWTrackFollower.speed jumps, per-tick diagnostic snapshot (speed + tunnel offset). Latches the CSWMiniGame pointer on first detect because the area chain (GetCurrentArea → GetClientArea → mini_game) churns during the race-start transition and stops resolving the minigame even while the race is still running — verified live by patch-20260524-163552.log where ENTER fired at T+0 and a spurious EXIT fired at T+125 ms even though the actual race continued for 40 more seconds.

## Declarations (in source order)

- L254 `inline float ShiftGateSpeed(int gear)`
- L428 `void* ReadMiniGameViaArea()`: Read CSWCArea.mini_game via the player-area chain.
- L441 `bool LatchedStillValid()`: Validate a latched CSWMiniGame pointer by re-reading its vtable and comparing against the value captured at latch time.
- L502 `bool ReadWorldClockMs(void* server, uint32_t& outMs, uint32_t& outMph)`: Current world-clock time-of-day in ms (the value GetTime* read) + the calendar's minutes-per-hour.
- L554 `void TickRaceTimer(void* miniGame)`: Per-tick: cache the start stamp once, then freeze the elapsed at each finish-band tick.
- L672 `void EmitDiagnosticDump(void* miniGame)`
- L704 `void EmitEntrySummary(void* miniGame)`: Brief per-entry summary line.
- L721 `void EmitDiagSnapshot(void* miniGame)`
- L755 `void AnnounceGear(int gear, const char* why)`: Speak the gear the bike is now in and re-arm the shift-ready cue for it.
- L768 `void TickGearWatch(void* miniGame)`
- L825 `void TickHitPoints(void* miniGame)`
- L854 `void DecideWreckAtFinish(void* miniGame)`: At the track's own race-end flag: finish or wreck?
- L902 `void TickStuckHint(void* miniGame)`
- L963 `void TickJumpWatch(void* miniGame)`
- L1006 `void TickShiftReady(void* miniGame)`: "You can shift now" cue (Option A — exact onfire.ncs gate, see kShiftGateSpeed).
- L1056 `void AnnounceEntry()`
- L1075 `void SpeakRaceEndMessage()`
- L1121 `void FlushRaceEndSpeech(bool force)`: Speaks the pending race time once its delay has elapsed (force = the race module is exiting: speak whatever is pending now rather than lose it).
- L1134 `void HandleEnter(void* mg)`
- L1182 `void HandleExit()`
- L1208 `bool RunPlayerSlotScript(uintptr_t runner, const char* what)`: Run one of the K2 player bike's slot scripts through the engine's own runner, mirroring the guards the client input handler applies (a live swoop-type minigame with a player).
- L1228 `bool IsActive()`
- L1230 `bool TryShift()`
- L1251 `bool TryJump()`
- L1255 `void Tick()`

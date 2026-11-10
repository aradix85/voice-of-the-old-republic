# minigame_swoop_race_cues.cpp (339 lines)

Source: `patches/Accessibility/minigame_swoop_race_cues.cpp`

The swoop race's spoken cues during a race: gear changes, when to shift, hit points, a stuck bike, and jumps.

## Declarations (in source order)

- L113 `inline float ShiftGateSpeed(int gear)`
- L124 `void AnnounceGear(int gear, const char* why)`: Speak the gear the bike is now in and re-arm the shift-ready cue for it.
- L139 `void TickGearWatch(void* miniGame)`
- L196 `void TickHitPoints(void* miniGame)`
- L213 `void TickStuckHint(void* miniGame)`
- L265 `void TickJumpWatch(void* miniGame)`
- L308 `void TickShiftReady(void* miniGame)`: "You can shift now" cue (Option A — exact onfire.ncs gate, see kShiftGateSpeed).

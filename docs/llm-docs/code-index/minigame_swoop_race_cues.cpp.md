# minigame_swoop_race_cues.cpp (286 lines)

Source: `patches/Accessibility/minigame_swoop_race_cues.cpp`

The swoop race's cues during a race: gear changes, when to shift, hit points, a stuck bike, and jumps.

## Declarations (in source order)

- L87 `void AnnounceGear(int gear, const char* why)`: Speak the gear the bike is now in and re-arm the shift-ready cue for it.
- L100 `void TickGearWatch(void* miniGame)`
- L151 `void TickHitPoints(void* miniGame)`: Hit points, log only.
- L168 `void TickStuckHint(void* miniGame)`
- L219 `void TickJumpWatch(void* miniGame)`
- L257 `void TickShiftReady(void* miniGame)`: The "you can shift now" cue (see kShiftGateSpeed): one cue the frame the current speed first crosses the gate for the current gear, i.e.

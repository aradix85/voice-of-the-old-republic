# minigame_swoop_race_cues.cpp (301 lines)

Source: `patches/Accessibility/minigame_swoop_race_cues.cpp`

The swoop race's cues during a race: gear changes, when to shift, hit points, a stuck bike, and jumps.

## Declarations (in source order)

- L88 `void AnnounceGear(int gear, const char* why)`: Speak the gear the bike is now in and re-arm the shift-ready cue for it.
- L101 `void TickGearWatch(void* miniGame)`
- L152 `void TickHitPoints(void* miniGame)`: Hit points, log only.
- L169 `void TickStuckHint(void* miniGame)`
- L220 `void TickJumpWatch(void* miniGame)`
- L258 `void TickShiftReady(void* miniGame)`: The "you can shift now" cue (see kShiftGateSpeed): one cue the frame the current speed first crosses the gate for the current gear, i.e.
- L289 `void TickAutoShift()`: The automatic pilot's shifts: the launch out of gear 0, and each gear as soon as TickShiftReady has seen its gate.

# minigame_swoop_race_timer.cpp (321 lines)

Source: `patches/Accessibility/minigame_swoop_race_timer.cpp`

The swoop race's clock and finish: the race time from the world clock, whether the bike wrecked at the finish, and the end-of-race line.

## Declarations (in source order)

- L97 `bool ReadWorldClockMs(void* server, uint32_t& outMs, uint32_t& outMph)`: Current world-clock time-of-day in ms (the value GetTime* read) + the calendar's minutes-per-hour.
- L131 `void DecideWreckAtFinish(void* miniGame)`: At the track's own race-end flag: finish or wreck?
- L163 `void TickRaceTimer(void* miniGame)`: Per-tick: cache the start stamp once, then freeze the elapsed at each finish-band tick.
- L265 `void SpeakRaceEndMessage()`
- L311 `void FlushRaceEndSpeech(bool force)`: Speaks the pending race time once its delay has elapsed (force = the race module is exiting: speak whatever is pending now rather than lose it).

# minigame_swoop_race_timer.cpp (299 lines)

Source: `patches/Accessibility/minigame_swoop_race_timer.cpp`

The swoop race's clock and finish: the race time from the world clock, whether the bike wrecked at the finish, and the end-of-race line.

## Declarations (in source order)

- L84 `bool ReadWorldClockMs(void* server, uint32_t& outMs, uint32_t& outMph)`: Current world-clock time-of-day in ms (the value GetTime* read) + the calendar's minutes-per-hour.
- L117 `void DecideWreckAtFinish(void* miniGame)`: At the track's own race-end flag: finish or wreck?
- L149 `void TickRaceTimer(void* miniGame)`: Per tick while the latch is alive: detect the finish, cache the start stamp once, and keep the elapsed current until the finish freezes it.
- L247 `void SpeakRaceEndMessage()`
- L289 `void FlushRaceEndSpeech(bool force)`: Speaks the pending race time once its delay has elapsed (force = the race module is exiting: speak whatever is pending now rather than lose it).

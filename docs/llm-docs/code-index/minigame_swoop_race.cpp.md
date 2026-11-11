# minigame_swoop_race.cpp (223 lines)

Source: `patches/Accessibility/minigame_swoop_race.cpp`

Swoop race: entry and exit, the shift and jump keys, and the per-tick dispatch to the cues, the race clock and the spatial audio.

The CSWMiniGame pointer is latched on first detect: the area chain (GetCurrentArea -> GetClientArea -> mini_game) stops resolving during the race-start transition while the race runs on, so the chain alone would announce a false exit a few ticks after the entry.

## Declarations (in source order)

- L55 `bool LatchedStillValid()`: True iff the latched CSWMiniGame is still readable and its vtable is the one captured at latch time.
- L63 `void AnnounceEntry()`: The opener and the key list go out as ONE interrupting utterance, so the second half cannot cut off the first.
- L77 `void HandleEnter(void* mg)`
- L92 `void HandleExit()`
- L106 `bool RunPlayerSlotScript(uintptr_t runner, const char* what)`: Run one of the K2 player bike's slot scripts through the engine's own runner, with the guards the client input handler applies (a live swoop-type minigame with a player).
- L126 `bool IsActive()`
- L128 `bool TryShift()`
- L149 `bool TryJump()`
- L153 `void Tick()`

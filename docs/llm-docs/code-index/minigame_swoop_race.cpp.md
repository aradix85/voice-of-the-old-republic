# minigame_swoop_race.cpp (179 lines)

Source: `patches/Accessibility/minigame_swoop_race.cpp`

Swoop race: entry and exit, the shift and jump keys, and the per-tick dispatch to the cues, the race clock and the spatial audio.

The CSWMiniGame pointer is latched on first detect: the area chain (GetCurrentArea -> GetClientArea -> mini_game) stops resolving during the race-start transition while the race runs on, so the chain alone would announce a false exit a few ticks after the entry.

## Declarations (in source order)

- L53 `void AnnounceEntry()`: The opener and the key list go out as ONE interrupting utterance, so the second half cannot cut off the first.
- L67 `void HandleEnter()`
- L81 `void HandleExit()`
- L95 `bool RunPlayerSlotScript(uintptr_t runner, const char* what)`: Run one of the K2 player bike's slot scripts through the engine's own runner, with the guards the client input handler applies (a live swoop-type minigame with a player).
- L115 `bool IsActive()`
- L117 `bool TryShift()`
- L138 `bool TryJump()`
- L142 `void Tick()`

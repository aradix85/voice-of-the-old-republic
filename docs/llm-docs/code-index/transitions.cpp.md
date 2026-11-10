# transitions.cpp (460 lines)

Source: `patches/Accessibility/transitions.cpp`

Area and region changes, spoken: the per-tick tracker that notices a new area or a new room_topology cluster, waits until the change is stable, and hands it to the room-speech and landmark parts of the module.

## Declarations (in source order)

- L77 `void ResetAreaState()`: Everything the module remembers about the current area.
- L88 `void SpeakArea(void* area)`
- L108 `void Tick()`
- L453 `bool IsWorldSpeechGated()`

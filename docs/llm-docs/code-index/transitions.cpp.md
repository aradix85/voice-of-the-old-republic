# transitions.cpp (509 lines)

Source: `patches/Accessibility/transitions.cpp`

Area and region changes, spoken: the per-tick tracker that notices a new area or a new room_topology cluster, waits until the change is stable, and hands it to the room-speech and landmark parts of the module.

## Declarations (in source order)

- L91 `void SpeakArea(void* area)`
- L114 `bool IsWorldSpeechGatedImpl()`: Forward declaration of the public predicate (defined at file scope below namespace).
- L121 `void Tick()`
- L504 `bool IsWorldSpeechGated()`

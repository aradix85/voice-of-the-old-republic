# pad_input_poll.cpp (371 lines)

Source: `patches/Accessibility/pad_input_poll.cpp`

Gamepad entry points of the input-poll slot: edge-detect the sampled button word (PollButtons), the buttons both games read from XInput (PollXInputButtons) and the trigger layer's solo jobs (PollTriggers).

## Declarations (in source order)

- L65 `void PollButtons()`
- L188 `bool PeekModifierHeld()`
- L190 `void PollXInputButtons()`
- L326 `void PollTriggers()`

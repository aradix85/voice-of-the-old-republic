# pad_input_poll.cpp (369 lines)

Source: `patches/Accessibility/pad_input_poll.cpp`

Gamepad entry points of the input-poll slot: edge-detect the sampled button word (PollButtons), the buttons both games read from XInput (PollXInputButtons) and the trigger layer's solo jobs (PollTriggers).

## Declarations (in source order)

- L65 `void PollButtons()`
- L186 `bool PeekModifierHeld()`
- L188 `void PollXInputButtons()`
- L324 `void PollTriggers()`

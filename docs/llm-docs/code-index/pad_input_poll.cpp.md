# pad_input_poll.cpp (375 lines)

Source: `patches/Accessibility/pad_input_poll.cpp`

Gamepad entry points of the input-poll slot: edge-detect the sampled button word (PollButtons), the buttons both games read from XInput (PollXInputButtons) and the trigger layer's solo jobs (PollTriggers).

## Declarations (in source order)

- L65 `void PollButtons()`
- L189 `bool PeekModifierHeld()`
- L191 `void PollXInputButtons()`
- L330 `void PollTriggers()`

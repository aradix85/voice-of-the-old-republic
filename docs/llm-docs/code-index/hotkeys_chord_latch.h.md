# hotkeys_chord_latch.h (63 lines)

Source: `patches/Accessibility/hotkeys_chord_latch.h`

Whose press is it when a chord's key reaches the engine after the modifier is up again? The engine reads buffered, modifier-blind presses; the mod reads the live keyboard. A quick Ctrl+L can have Ctrl released by the time the engine's L arrives, the more so when the hotkey itself takes long (Ctrl+L plans a walk), and the engine then opens its own screen for bare L.

The latch pairs the two views of one physical press: the poll notes a chord going down, the input hook notes an engine press it swallowed with the modifier still held, and whichever comes second finds the first. Short-lived, so it can never take a later bare press. Engine-free.

## Declarations (in source order)

- L18 `class ChordLatch`

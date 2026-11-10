# pad_input.cpp (573 lines)

Source: `patches/Accessibility/pad_input.cpp`

Gamepad input: loading XInput, the per-tick sample (triggers, sticks, buttons, which slot answers), pad presence, and whether the engine's own pad path is alive. See pad_input.h for why this is one module, and why KOTOR 1 and KOTOR 2 share every binding but not their event source.

## Declarations (in source order)

- L146 `bool SampleStick(int x, int y, float& outX, float& outY)`: One stick's sample: true past the dead zone, with the direction normalised into outX / outY; zero inside it.
- L162 `void ReleaseSampledState()`: Everything the sample holds, read as released: the slot it came from is gone or no longer ours.
- L172 `void NoteEnginePadLive(const char* why)`
- L185 `void EnsureXInput()`
- L221 `bool AdoptLivelierSlot(ULONGLONG now)`: The idle-slot guard — see kLatchIdleMs for what it is defending against.
- L316 `void NoteSeen()`: Only the two engine-native-discriminated event shapes are allowed to set this: a stick axis carrying val = +/-1 (the engine's own preamble keys on exactly that), and a D-Pad code (F9..F12, unbound in stock [Keymapping...
- L331 `bool InWorldSurface()`: Surfaces "World" means the pad is driving the game, not a panel: the player exists and no engine panel owns the foreground.
- L351 `void NoteNavPress()`: The cancel a pad press has no keystroke to trigger.
- L359 `bool EnginePadDead()`
- L361 `bool LeftTriggerHeld()`
- L363 `bool RightTriggerHeld()`
- L365 `bool Connected()`
- L371 `void Tick()`
- L554 `bool StickMoving()`
- L556 `bool InWorld()`
- L558 `bool StickVector(float& outX, float& outY)`
- L565 `bool RightStickVector(float& outX, float& outY)`

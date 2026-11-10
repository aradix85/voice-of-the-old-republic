# pad_input.cpp (583 lines)

Source: `patches/Accessibility/pad_input.cpp`

Gamepad input: loading XInput, the per-tick sample (triggers, sticks, buttons, which slot answers), pad presence, and whether the engine's own pad path is alive. See pad_input.h for why this is one module, and why KOTOR 1 and KOTOR 2 share every binding but not their event source.

## Declarations (in source order)

- L144 `void NoteEnginePadLive(const char* why)`
- L157 `void EnsureXInput()`
- L194 `bool AdoptLivelierSlot(ULONGLONG now)`: The idle-slot guard — see kLatchIdleMs for what it is defending against.
- L298 `void NoteSeen()`: Only the two engine-native-discriminated event shapes are allowed to set this: a stick axis carrying val = +/-1 (the engine's own preamble keys on exactly that), and a D-Pad code (F9..F12, unbound in stock [Keymapping...
- L313 `bool InWorldSurface()`: Surfaces "World" means the pad is driving the game, not a panel: the player exists and no engine panel owns the foreground.
- L333 `void NoteNavPress()`: The cancel a pad press has no keystroke to trigger.
- L341 `bool EnginePadDead()`
- L343 `bool LeftTriggerHeld()`
- L345 `bool RightTriggerHeld()`
- L347 `bool Connected()`
- L353 `void Tick()`
- L564 `bool StickMoving()`
- L566 `bool InWorld()`
- L568 `bool StickVector(float& outX, float& outY)`
- L575 `bool RightStickVector(float& outX, float& outY)`

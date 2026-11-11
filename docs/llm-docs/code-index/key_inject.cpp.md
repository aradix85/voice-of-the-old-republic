# key_inject.cpp (61 lines)

Source: `patches/Accessibility/key_inject.cpp`

(From key_inject.h.) Synthetic keystrokes at the SCANCODE level — the one way this mod makes the engine act as though a key were pressed.

Why scancodes and not VKs ------------------------- KOTOR reads the keyboard through DirectInput, which sees physical scancodes only. A plain-VK SendInput is invisible to it. This is the mechanism camera_orient's snap-turn uses to drive the engine's own turn axis; the swoop race and the bridge's key action use it too.

Deliberately policy-free ------------------------ No foreground gate, no edge tracking, no repeat handling. Callers own all of that, because they differ: camera_orient holds a key across ticks with its own predictive release and gates on hotkeys::IsForegroundGame() before it ever presses. A gate inside here would swallow the RELEASE of a key held when focus was lost — the one failure this module must never be able to cause.

## Declarations (in source order)

- L9 `void Send(int scancode, bool down, bool extended)`
- L20 `void Tap(int scancode, bool extended)`
- L33 `void ReleaseHeld()`
- L41 `void TapHeld(int scancode, bool extended)`
- L56 `void Tick()`

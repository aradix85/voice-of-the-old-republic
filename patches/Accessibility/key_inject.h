// Synthetic keystrokes at the SCANCODE level — the one way this mod makes the
// engine act as though a key were pressed.
//
// Why scancodes and not VKs
// -------------------------
// KOTOR reads the keyboard through DirectInput, which sees physical scancodes
// only. A plain-VK SendInput is invisible to it. This is the mechanism
// camera_orient's snap-turn uses to drive the engine's own turn axis; the
// swoop race and the bridge's key action use it too.
//
// Deliberately policy-free
// ------------------------
// No foreground gate, no edge tracking, no repeat handling. Callers own all of
// that, because they differ: camera_orient holds a key across ticks with its
// own predictive release and gates on hotkeys::IsForegroundGame() before it
// ever presses. A gate inside
// here would swallow the RELEASE of a key held when focus was lost — the one
// failure this module must never be able to cause.

#pragma once

namespace acc::key_inject {

// Press (down = true) or release one DIK scancode.
//
// `extended` is for the keys that live behind the E0 prefix — the arrow
// cluster, right Ctrl/Alt, numpad Enter. DirectInput reports those as
// DIK_<key> = base | 0x80; SendInput wants the BASE scancode plus
// KEYEVENTF_EXTENDEDKEY, so pass 0x48 + extended for Up, not 0xC8.
void Send(int scancode, bool down, bool extended = false);

// Press and release in one call, for a key that only needs to fire once.
void Tap(int scancode, bool extended = false);

}  // namespace acc::key_inject

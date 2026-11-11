# menus_keybinds.cpp (402 lines)

Source: `patches/Accessibility/menus_keybinds.cpp`

Mod keybind configurator — see menus_keybinds.h for the design summary.

## Declarations (in source order)

- L24 `const char* T(S id)`
- L125 `constexpr Category MakeCat(S name, const ActionEntry (&arr)[N])`
- L153 `const ActionEntry* FindEntry(A action)`
- L163 `void SpeakCategory(bool interrupt)`: ----- Speech ---------------------------------------------------------------
- L172 `void SpeakActionRow(bool interrupt)`
- L183 `void SnapshotKeys()`: ----- Capture --------------------------------------------------------------
- L192 `bool IsCandidateVk(int vk)`: VKs that may NOT be captured as a binding key: mouse buttons, the modifier keys themselves (those form the combo, not the base key), the Windows keys, and Caps Lock.
- L207 `void ArmCapture()`
- L222 `void ApplyCapturedKey(int vk)`: Evaluate a freshly-pressed key as the candidate binding.
- L277 `const char* DisplayName(A action)`: ----- Public API -----------------------------------------------------------
- L282 `bool IsOpen()`
- L284 `void Open()`
- L299 `void Reset()`
- L308 `bool HandleInput(int keyCode)`
- L374 `void Tick()`

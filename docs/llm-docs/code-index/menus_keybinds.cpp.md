# menus_keybinds.cpp (404 lines)

Source: `patches/Accessibility/menus_keybinds.cpp`

Mod keybind configurator — see menus_keybinds.h for the design summary.

## Declarations (in source order)

- L24 `const char* T(S id)`
- L127 `constexpr Category MakeCat(S name, const ActionEntry (&arr)[N])`
- L155 `const ActionEntry* FindEntry(A action)`
- L165 `void SpeakCategory(bool interrupt)`: ----- Speech ---------------------------------------------------------------
- L174 `void SpeakActionRow(bool interrupt)`
- L185 `void SnapshotKeys()`: ----- Capture --------------------------------------------------------------
- L194 `bool IsCandidateVk(int vk)`: VKs that may NOT be captured as a binding key: mouse buttons, the modifier keys themselves (those form the combo, not the base key), the Windows keys, and Caps Lock.
- L209 `void ArmCapture()`
- L224 `void ApplyCapturedKey(int vk)`: Evaluate a freshly-pressed key as the candidate binding.
- L279 `const char* DisplayName(A action)`: ----- Public API -----------------------------------------------------------
- L284 `bool IsOpen()`
- L286 `void Open()`
- L301 `void Reset()`
- L310 `bool HandleInput(int keyCode)`
- L376 `void Tick()`

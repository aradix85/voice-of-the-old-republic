# menus_keybinds.cpp (405 lines)

Source: `patches/Accessibility/menus_keybinds.cpp`

Mod keybind configurator — see menus_keybinds.h for the design summary.

## Declarations (in source order)

- L24 `const char* T(S id)`
- L128 `constexpr Category MakeCat(S name, const ActionEntry (&arr)[N])`
- L156 `const ActionEntry* FindEntry(A action)`
- L166 `void SpeakCategory(bool interrupt)`: ----- Speech ---------------------------------------------------------------
- L175 `void SpeakActionRow(bool interrupt)`
- L186 `void SnapshotKeys()`: ----- Capture --------------------------------------------------------------
- L195 `bool IsCandidateVk(int vk)`: VKs that may NOT be captured as a binding key: mouse buttons, the modifier keys themselves (those form the combo, not the base key), the Windows keys, and Caps Lock.
- L210 `void ArmCapture()`
- L225 `void ApplyCapturedKey(int vk)`: Evaluate a freshly-pressed key as the candidate binding.
- L280 `const char* DisplayName(A action)`: ----- Public API -----------------------------------------------------------
- L285 `bool IsOpen()`
- L287 `void Open()`
- L302 `void Reset()`
- L311 `bool HandleInput(int keyCode)`
- L377 `void Tick()`

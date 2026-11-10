# examine_view.cpp (169 lines)

Source: `patches/Accessibility/examine_view.cpp`

(From examine_view.h.) Examine list view.

Synthetic in-DLL arrow-navigable list of pre-composed rows. KOTOR 1's CSWGuiExamine is a generic TLK message box, not a rich examine panel, so we build our own from direct field reads + engine name accessors.

User contract while armed: Open     — captures cycle/LastTarget focus, builds rows, speaks opener + row 0. Up/Down  — step through rows (Name, Faction, HP, Distance, Weapon, Effects×N, Feats×M). HP and distance refresh on step. Enter    — close (read-only, no commit). Esc      — close.

Self-disarms when the target becomes unresolvable.

Input routing lives in interact_hotkey.cpp.

## Declarations (in source order)

- L15 `void SpeakRow(int idx)`
- L26 `bool IsActive()`
- L28 `void ForceDisarm(const char* reason)`
- L40 `bool Open()`
- L89 `bool HandleInputEvent(int code, int value)`
- L139 `void Tick()`: Engine-panel open/close logger.
- L150 `void PollWin32Hotkey()`

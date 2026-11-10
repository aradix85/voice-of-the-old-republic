# menus_dispatch.cpp (627 lines)

Source: `patches/Accessibility/menus_dispatch.cpp`

GUI input dispatch + focus-change hook handlers.

Split out of menus.cpp by the Phase-1 structure pass (refactoring candidate 1). Two engine hooks live here:

* OnHandleFocusChange (CSWGuiControl::HandleFocusChange) - log only. * OnHandleInputEvent (CSWGuiManager::HandleInputEvent) - the ordered gate list every GUI keypress runs through. This is the single largest function in the mod and the reason the file exists: it was ~40% of menus.cpp and has no state of its own beyond the two menus.cpp-owned flags it reads through menus_internal.h.

The definitions are verbatim from menus.cpp. Gate ORDER is behaviour - each subsystem gets first refusal in a specific sequence - so nothing in this file may be reordered without an in-game input pass.

## Declarations (in source order)

- L68 `extern "C" void __cdecl OnHandleFocusChange(void* thisPtr, int param_1)`: CSWGuiControl::HandleFocusChange — hooked mid-function at 0x41896b.
- L93 `extern "C" int __cdecl OnHandleInputEvent(void* thisPtr, int param_1, int param_2)`: CSWGuiManager::HandleInputEvent — hooked mid-function at 0x0040c907.

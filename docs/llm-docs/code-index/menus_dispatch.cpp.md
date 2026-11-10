# menus_dispatch.cpp (620 lines)

Source: `patches/Accessibility/menus_dispatch.cpp`

GUI input dispatch + focus-change hook handlers. Two engine hooks:

* OnHandleFocusChange (CSWGuiControl::HandleFocusChange) - log only. * OnHandleInputEvent (CSWGuiManager::HandleInputEvent) - the ordered gate list every GUI keypress runs through. It reads two menus.cpp flags through menus_internal.h.

Gate ORDER is behaviour - each subsystem gets first refusal in a specific sequence - so nothing in this file may be reordered without an in-game input pass.

## Declarations (in source order)

- L61 `extern "C" void __cdecl OnHandleFocusChange(void* thisPtr, int param_1)`: CSWGuiControl::HandleFocusChange — hooked mid-function at 0x41896b.
- L86 `extern "C" int __cdecl OnHandleInputEvent(void* thisPtr, int param_1, int param_2)`: CSWGuiManager::HandleInputEvent — hooked mid-function at 0x0040c907.

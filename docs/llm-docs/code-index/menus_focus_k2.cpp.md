# menus_focus_k2.cpp (235 lines)

Source: `patches/Accessibility/menus_focus_k2.cpp`

KOTOR 2's GUI hooks. The handlers are the ones KOTOR 1 hooks directly (menus_setactive.cpp, menus_dispatch.cpp); this file holds what KOTOR 2 needs before it can reach them.

Frame arithmetic: KOTOR 2's GUI code is compiled unoptimised. It keeps a frame pointer and reloads `this` and the arguments from the frame at every use, where KOTOR 1 holds them in registers. Each hook therefore passes EBP and the wrapper reads the arguments itself. Declaring them as stack parameters is not an option: KPatchManager's esp+X parameter source emits LEA instead of MOV and hands back addresses (docs/upstream-prs.md, PR-2).

For focus, two more things: the hook sits ahead of SetActiveControl's own `if (active != param_1)` test, so calls that change nothing arrive here and are dropped; and the mouse cursor is kept on the focused control, so that hover stops overriding the keyboard.

## Declarations (in source order)

- L48 `bool CallerIsFocusChange(void* caller)`
- L81 `void WarpCursorToControl(void* control)`: Park the mouse cursor on a control.
- L109 `void HandleSetActiveControl(void* panel, void* control, void* caller)`: One SetActiveControl call.
- L144 `bool FramePtr(void* ebp, ptrdiff_t disp, void** out)`: The frame slot [EBP + disp].
- L148 `bool FrameInt(void* ebp, ptrdiff_t disp, int* out)`
- L156 `bool StoreInputCode(void* mgr, int code)`: this->input_code = code on a CSWGuiManager.
- L168 `extern "C" void __cdecl OnSetActiveControlK2(void* ebp)`: CSWGuiPanel::SetActiveControl @0x0040EC00, hooked at 0x0040EC09, after the prologue has stored `this`.
- L194 `extern "C" int __cdecl OnHandleInputEventK2(void* ebp)`: CSWGuiManager::HandleInputEvent @0x00410AA0, hooked at 0x00410AC8, after the SEH prologue has stored `this`.
- L213 `extern "C" void __cdecl OnHandleFocusChangeK2(void* thisCtrl, void* ebp)`: CSWGuiControl::HandleFocusChange @0x00418FE0, hooked at 0x00418FE6, before `this` is stored, so the hook passes ECX and EBP.
- L226 `extern "C" void __cdecl OnListBoxSetActiveControlK2(void* listBox, void* ebp)`: CSWGuiListBox::SetActiveControl's implementation @0x0041E9A0 (the vtable slot 0x0041FEE0 is a forwarder to it, so hooking the body catches both routes), hooked at 0x0041E9A4, before `this` is stored.

# gui_forward_guard_k2.cpp (75 lines)

Source: `patches/Accessibility/gui_forward_guard_k2.cpp`

KOTOR 2 empty-listbox navigation crash guard.

The engine's GUI event-forward function (0x00412C00) caches the event's two params into this+0/+4, updates the child at this+0x24, then forwards the event to the focused child at this+0x10 through that child's vtable slot 4:

00412c94  cmp [ecx+0x10], 0        ; the engine's own guard... 00412c98  je  0x412cbe             ; ...jumps away only when null ... 00412cae  mov edx, [edx]           ; child->vtable 00412cb0  mov eax, [edx+0x10]      ; vtable slot 4 00412cb3  call eax                 ; forward the event

On the save/load screen without saved games the list is empty and this+0x10 is a stale pointer into freed memory that now holds string data. It is not null, so the engine's check passes, the "vtable" read yields garbage and `call eax` jumps into the middle of a string (a crash dump had EIP = 0x65726944, ASCII "Dire").

A hook, not input suppression: with a listbox focused the engine routes the arrow keys straight to it, past CSWGuiManager::HandleInputEvent, so the manager hook never sees the fatal press.

KOTOR 1 does not need it: its save/load listbox handling (menus_listbox.cpp, SaveLoadMatches) owns Up/Down on that screen, so the arrows never reach the engine's empty-list navigation.

The hook sits on the engine's null check (0x00412C94) and cuts the 6-byte `cmp [ecx+0x10],0 ; je <skip>`. It answers with validity where the engine asks for non-null:

1 -> skip the forward: the engine's own "child did not handle it" path (0x00412CBE). For a null child, an unreadable one, and a vtable slot 4 that does not point at code. 0 -> go on and forward the event.

## Declarations (in source order)

- L45 `bool PointsToCode(const void* p)`: Committed, executable memory: a plausible function pointer.
- L58 `extern "C" int __cdecl OnGuiForwardEventGuardK2(void* self)`: `self` is ECX at the cut: the forwarding container.

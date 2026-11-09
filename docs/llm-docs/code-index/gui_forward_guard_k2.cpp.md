# gui_forward_guard_k2.cpp (113 lines)

Source: `patches/Accessibility/gui_forward_guard_k2.cpp`

KOTOR 2 empty-listbox navigation crash guard.

The defect ---------- The engine's GUI event-forward function (KOTOR 2 Steam 0x00412C00, GOG 0x00911080) caches the event's two params into this+0/+4, updates the child at this+0x24, then FORWARDS the event to the focused child at this+0x10 by calling that child's vtable slot 4:

0041 2c94  cmp [ecx+0x10], 0        ; the engine's own guard... 0041 2c98  je  0x412cbe             ; ...jumps away only when NULL ... 0041 2cae  mov edx, [edx]           ; child->vtable 0041 2cb0  mov eax, [edx+0x10]      ; vtable slot 4 0041 2cb3  call eax                 ; <== forward the event

On the save/load screen with NO saved games the list is empty, and this+0x10 is a STALE pointer into freed memory that now holds string data. It is non-null, so the engine's `!= 0` check passes; the "vtable" read then yields garbage and `call eax` jumps into the middle of a string (the crash dump caught EIP = 0x65726944, ASCII "Dire"). Classic "non-null is not proof of life" — the same class as our own chain crashes, but this copy lives in the engine.

Why only KOTOR 2, and why a hook rather than input suppression -------------------------------------------------------------- When a listbox is the focused control the engine routes arrow keys straight to it, BYPASSING CSWGuiManager::HandleInputEvent (the manager hook we watch) — the crash's whole call stack is engine code with no accessibility frame, and no Menus.Input line is logged for the fatal press. So consuming the key in our manager dispatcher cannot help; the guard must sit on the crashing function itself.

KOTOR 1 does not need it: its save/load listbox spec (menus_listbox.cpp, SaveLoadMatches — deliberately K1-only) owns Up/Down on that screen, so the arrows never reach the engine's native empty-list nav there. KOTOR 2 keeps ordinary chain navigation over rows + buttons instead, which leaves the engine's native path live — hence the crash and this guard. The twin function on KOTOR 1 is the optimised (register-based) codegen of the same logic and is not byte-locatable from this one; it is also unreachable, so it is intentionally not hooked.

The guard --------- Hooked at the engine's own null check (Steam 0x00412C94, GOG translated), cutting the 6-byte `cmp [ecx+0x10],0 ; je <skip>`. We re-implement the check with real validity instead of mere non-nullness:

return 1  -> skip the forward: jump to consumed_exit (the engine's own "child did not handle it" path, Steam 0x00412CBE). Taken when this+0x10 is null OR its vtable slot 4 does not point at code. return 0  -> proceed: natural resume forwards the event as the engine would, for a real focused child.

The whole pointer chain is read under SEH, and the final target is validated with VirtualQuery (build-agnostic: a real method lands in committed executable memory; the "Dire…" garbage does not). A fault anywhere in the walk resolves to skip, never to a crash.

## Declarations (in source order)

- L72 `bool PointsToCode(const void* p)`: True only when `p` is a committed, executable address — i.e.
- L88 `extern "C" int __cdecl OnGuiForwardEventGuardK2(void* self)`: Parameter: ecx = `this` (the forwarding container) at the cut.

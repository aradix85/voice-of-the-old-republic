// KOTOR 2 empty-listbox navigation crash guard.
//
// The engine's GUI event-forward function (0x00412C00) caches the event's two
// params into this+0/+4, updates the child at this+0x24, then forwards the
// event to the focused child at this+0x10 through that child's vtable slot 4:
//
//   00412c94  cmp [ecx+0x10], 0        ; the engine's own guard...
//   00412c98  je  0x412cbe             ; ...jumps away only when null
//   ...
//   00412cae  mov edx, [edx]           ; child->vtable
//   00412cb0  mov eax, [edx+0x10]      ; vtable slot 4
//   00412cb3  call eax                 ; forward the event
//
// On the save/load screen without saved games the list is empty and this+0x10
// is a stale pointer into freed memory that now holds string data. It is not
// null, so the engine's check passes, the "vtable" read yields garbage and
// `call eax` jumps into the middle of a string (a crash dump had EIP =
// 0x65726944, ASCII "Dire").
//
// A hook, not input suppression: with a listbox focused the engine routes the
// arrow keys straight to it, past CSWGuiManager::HandleInputEvent, so the
// manager hook never sees the fatal press.
//
// KOTOR 1 does not need it: its save/load listbox handling (menus_listbox.cpp,
// SaveLoadMatches) owns Up/Down on that screen, so the arrows never reach the
// engine's empty-list navigation.
//
// The hook sits on the engine's null check (0x00412C94) and cuts the 6-byte
// `cmp [ecx+0x10],0 ; je <skip>`. It answers with validity where the engine
// asks for non-null:
//
//   1 -> skip the forward: the engine's own "child did not handle it" path
//        (0x00412CBE). For a null child, an unreadable one, and a vtable
//        slot 4 that does not point at code.
//   0 -> go on and forward the event.

#include <windows.h>

#include "engine_reads.h"
#include "log.h"

namespace {

// Committed, executable memory: a plausible function pointer.
bool PointsToCode(const void* p) {
    if (!p) return false;
    MEMORY_BASIC_INFORMATION mbi;
    if (::VirtualQuery(p, &mbi, sizeof(mbi)) != sizeof(mbi)) return false;
    if (mbi.State != MEM_COMMIT) return false;
    const DWORD exec = PAGE_EXECUTE | PAGE_EXECUTE_READ |
                       PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
    return (mbi.Protect & exec) != 0;
}

}  // namespace

// `self` is ECX at the cut: the forwarding container.
extern "C" int __cdecl OnGuiForwardEventGuardK2(void* self) {
    using acc::engine::TryReadPtr;

    void* child = nullptr;
    if (!self || !TryReadPtr(self, 0x10, &child) || !child) return 1;

    void* vtable = nullptr;
    void* target = nullptr;
    if (TryReadPtr(child, 0, &vtable) && TryReadPtr(vtable, 0x10, &target) &&
        PointsToCode(target)) {
        return 0;
    }
    acclog::Write("SaveLoad", "GUI-forward guard: child %p of %p has no code at "
                              "vtable slot 4 (%p); forward skipped",
                  child, self, target);
    return 1;
}

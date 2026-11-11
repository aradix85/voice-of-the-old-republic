// KOTOR 2's GUI hooks. The handlers are the ones KOTOR 1 hooks directly
// (menus_setactive.cpp, menus_dispatch.cpp); this file holds what KOTOR 2 needs
// before it can reach them.
//
// Frame arithmetic: KOTOR 2's GUI code is compiled unoptimised. It keeps a
// frame pointer and reloads `this` and the arguments from the frame at every
// use, where KOTOR 1 holds them in registers. Each hook therefore passes EBP
// and the wrapper reads the arguments itself. Declaring them as stack
// parameters is not an option: KPatchManager's esp+X parameter source emits
// LEA instead of MOV and hands back addresses (docs/upstream-prs.md, PR-2).
//
// For focus, two more things: the hook sits ahead of SetActiveControl's own
// `if (active != param_1)` test, so calls that change nothing arrive here and
// are dropped; and the mouse cursor is kept on the focused control, so that
// hover stops overriding the keyboard.

#include <windows.h>
#include <cstddef>
#include <cstdint>

#include "engine_manager.h"
#include "engine_offsets.h"
#include "engine_reads.h"
#include "engine_addr.h"
#include "log.h"

extern "C" void __cdecl OnSetActiveControl(void* panel, void* newControl);
extern "C" int __cdecl OnHandleInputEvent(void* thisPtr, int param_1,
                                          int param_2);
extern "C" void __cdecl OnHandleFocusChange(void* thisPtr, int param_1);
extern "C" void __cdecl OnListBoxSetActiveControl(void* listBox, void* newRow,
                                                  int param2);

namespace {

using acc::engine::GetGuiManager;
using acc::engine::ReadControlId;
using acc::engine::TryReadPtr;
using acc::engine::TryReadU32;

// CSWGuiControl::HandleFocusChange and its body length. It calls
// SetActiveControl itself for the control under the cursor, so its calls
// arrive at the focus hook looking like a navigation when they are the hover
// path re-asserting itself.
const uintptr_t kAddrControlHandleFocusChange = acc::addr::Kotor2Only(0x00418FE0);
constexpr size_t kHandleFocusChangeSize = 116;

bool CallerIsFocusChange(void* caller) {
    const uintptr_t va = reinterpret_cast<uintptr_t>(caller);
    return va >= kAddrControlHandleFocusChange &&
           va < kAddrControlHandleFocusChange + kHandleFocusChangeSize;
}

// CSWGuiManager's hover routine: runs the hit test at the manager's stored
// cursor (manager+0, manager+4) and updates the hovered control (manager+8).
// It has no KOTOR 1 twin: KOTOR 1's HandleMouseMove takes x and y as
// arguments.
const uintptr_t kAddrK2ManagerRehover = acc::addr::Kotor2Only(0x00413C50);
typedef void(__thiscall* PFN_Rehover)(void*);

// The re-hover below can drive a SetActiveControl straight back through the
// focus hook. The latch keeps that nested event from being treated as a
// navigation.
bool s_inWarp = false;

// Park the mouse cursor on a control.
//
// The manager hit-tests at the cursor every frame and gives the control under
// it focus through HandleFocusChange, which overrides what the arrow keys just
// selected: focus snaps back to one button after every keypress.
//
// The engine's own MoveMouseToPosition (0x00414230) does not help on KOTOR 2:
// its CExoInput::SetMousePos writes only the engine's copy of the position,
// which the engine overwrites from the real mouse on the next frame. Writing
// manager+0/+4 alone fails for the same reason, and manager+8 only moves when
// the hit test runs. So: move the real cursor, store the position, re-hover.
//
// Control extents on KOTOR 2 are window-client pixels, not the GUI's virtual
// 640x480 space; the panel hit-test's `x -= (screenWidth - 640) / 2` applies
// to coordinates from elsewhere, not to extents.
void WarpCursorToControl(void* control) {
    if (s_inWarp) return;
    void* mgr = GetGuiManager();
    if (!mgr) return;
    s_inWarp = true;
    __try {
        const int* extent = reinterpret_cast<const int*>(
            static_cast<char*>(control) + kControlExtentOffset);
        const int cx = extent[0] + extent[2] / 2;   // left + width/2
        const int cy = extent[1] + extent[3] / 2;   // top + height/2

        POINT pt = { cx, cy };
        HWND hwnd = GetActiveWindow();
        if (hwnd) ClientToScreen(hwnd, &pt);
        SetCursorPos(pt.x, pt.y);

        int* cursor = static_cast<int*>(mgr);
        cursor[0] = cx;
        cursor[1] = cy;
        reinterpret_cast<PFN_Rehover>(kAddrK2ManagerRehover)(mgr);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        // A failed warp only means hover keeps fighting the keyboard.
    }
    s_inWarp = false;
}

// One SetActiveControl call. `caller` is its return address: which engine
// function set focus (docs/llm-docs/re/k2/k2-functions.csv maps it back).
void HandleSetActiveControl(void* panel, void* control, void* caller) {
    if (!control || s_inWarp) return;

    // A control that cannot be read is mid-teardown or not a control: neither
    // announced nor warped to.
    uint32_t vtable = 0;
    void* prevActive = nullptr;
    if (!TryReadU32(control, 0, &vtable) ||
        (panel && !TryReadPtr(panel, kPanelActiveControlOffset, &prevActive))) {
        acclog::Write("K2.Focus", "control=%p caller=%p unreadable, skipped",
                      control, caller);
        return;
    }

    const bool noop = prevActive == control;
    const bool fromFocusChange = CallerIsFocusChange(caller);
    acclog::Write("K2.Focus", "prev=%d -> id=%d vtable=%08x %s caller=%p%s",
                  prevActive ? ReadControlId(prevActive) : -1,
                  ReadControlId(control), vtable,
                  noop ? "[no-op]" : "[change]", caller,
                  fromFocusChange ? " [focus-change]" : "");
    if (noop) return;

    // Not on HandleFocusChange's own call: that would chase the cursor to where
    // hover already points. Once the cursor follows the keyboard, that call
    // targets the active control and is a no-op.
    //
    // Before the shared handler, so that the nested SetActiveControl the warp
    // can cause arrives while the latch is set and not in mid-announce.
    if (!fromFocusChange) WarpCursorToControl(control);

    OnSetActiveControl(panel, control);
}

// The frame slot [EBP + disp].
bool FramePtr(void* ebp, ptrdiff_t disp, void** out) {
    return TryReadPtr(static_cast<char*>(ebp) + disp, 0, out);
}

bool FrameInt(void* ebp, ptrdiff_t disp, int* out) {
    uint32_t value = 0;
    if (!TryReadU32(static_cast<char*>(ebp) + disp, 0, &value)) return false;
    *out = static_cast<int>(value);
    return true;
}

// this->input_code = code on a CSWGuiManager.
bool StoreInputCode(void* mgr, int code) {
    return acc::engine::TryWriteI32(mgr, kMgrInputCodeOffset, code);
}

}  // namespace

// CSWGuiPanel::SetActiveControl @0x0040EC00, hooked at 0x0040EC09, after the
// prologue has stored `this`.
//
//   [EBP-8] = this        (CSWGuiPanel*)
//   [EBP+4] = return address
//   [EBP+8] = param_1     (CSWGuiControl*, the newly active control)
extern "C" void __cdecl OnSetActiveControlK2(void* ebp) {
    void* panel = nullptr;
    void* control = nullptr;
    void* caller = nullptr;
    if (!ebp || !FramePtr(ebp, -8, &panel) || !FramePtr(ebp, 8, &control) ||
        !FramePtr(ebp, 4, &caller)) {
        return;
    }
    HandleSetActiveControl(panel, control, caller);
}

// CSWGuiManager::HandleInputEvent @0x00410AA0, hooked at 0x00410AC8, after the
// SEH prologue has stored `this`.
//
//   [EBP-0x68] = this      (CSWGuiManager*)
//   [EBP+8]    = param_1   (InputIndex key/button code)
//   [EBP+0xC]  = param_2   (state; 0 = release)
//
// The hook sets skip_original_bytes: the three cut instructions perform
// `this->input_code = param_1`, but the first of them loads EAX, and EAX is
// the consume-signal register KPatchManager's wrapper tests after replaying
// the cut. So the cut is not replayed and the store is made here.
//
// Non-zero = consumed. The wrapper then jumps to the function's common
// epilogue at 0x00410FA9, which restores FS:[0] before returning; the engine's
// own repeat-debounce paths jump to the same target from mid-body.
extern "C" int __cdecl OnHandleInputEventK2(void* ebp) {
    void* mgr = nullptr;
    int code = 0;
    int state = 0;
    if (!ebp || !FramePtr(ebp, -0x68, &mgr) || !mgr || !FrameInt(ebp, 8, &code) ||
        !FrameInt(ebp, 0xC, &state) || !StoreInputCode(mgr, code)) {
        return 0;
    }
    return OnHandleInputEvent(mgr, code, state);
}

// CSWGuiControl::HandleFocusChange @0x00418FE0, hooked at 0x00418FE6, before
// `this` is stored, so the hook passes ECX and EBP.
//
//   ECX      = this      (CSWGuiControl*)
//   [EBP+8]  = param_1   (0 = losing focus, non-zero = gaining)
//
// The cut replays MOV [EBP-0x10],ECX and CMP [EBP+8],0 after this returns, so
// the flags for the engine's JZ at 0x00418FED come from the replayed CMP.
extern "C" void __cdecl OnHandleFocusChangeK2(void* thisCtrl, void* ebp) {
    int gaining = 0;
    if (!thisCtrl || !ebp || !FrameInt(ebp, 8, &gaining)) return;
    OnHandleFocusChange(thisCtrl, gaining);
}

// CSWGuiListBox::SetActiveControl's implementation @0x0041E9A0 (the vtable
// slot 0x0041FEE0 is a forwarder to it, so hooking the body catches both
// routes), hooked at 0x0041E9A4, before `this` is stored.
//
//   ECX       = this      (CSWGuiListBox*)
//   [EBP+8]   = param_1   (CSWGuiControl*, the newly active row)
//   [EBP+0xC] = param_2   (int, the engine's play-sound flag)
extern "C" void __cdecl OnListBoxSetActiveControlK2(void* listBox, void* ebp) {
    void* newRow = nullptr;
    int playSound = 0;
    if (!listBox || !ebp || !FramePtr(ebp, 8, &newRow) ||
        !FrameInt(ebp, 0xC, &playSound)) {
        return;
    }
    OnListBoxSetActiveControl(listBox, newRow, playSound);
}

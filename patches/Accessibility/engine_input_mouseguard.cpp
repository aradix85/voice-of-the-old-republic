// The no-mouse crash guard on CExoRawInputInternal::InitializeDirectInputMouse
// and the teardown block behind it.

#include "engine_input_internal.h"

#include "engine_game.h"
#include "log.h"

#include <windows.h>
#include <intrin.h>  // _ReturnAddress names the failing mouse-init step

#include <cstring>

namespace acc::engine {

namespace {

// ---------------------------------------------------------------------------
// CExoRawInputInternal::InitializeDirectInputMouse — no-mouse crash guard
// ---------------------------------------------------------------------------
// Vanilla engine bug, present in both games. The function's whole job is
// IDirectInput8::CreateDevice(GUID_SysMouse, &this->mouse_device, NULL), and it
// loads the interface's vtable without ever testing the interface for NULL:
//
//     MOV EDX, [this + direct_input_interface]
//     MOV EDX, [EDX]          <-- AV when the interface is NULL
//     MOV EAX, [EDX + 0xc]    <-- IDirectInput8 vtable slot 3 = CreateDevice
//     CALL EAX
//
// The interface is NULL exactly when the engine has already torn DirectInput
// down. On any device bring-up failure it calls ShutDownDirectInput, which
// nulls the field — and then reports success to its caller anyway. The mouse
// device pointer is left NULL too, so the per-frame mouse update re-enters
// through the lazy "create the device if we haven't got one" path and
// dereferences the NULL interface. Uptime to the crash is however long the
// intro takes to reach the first main-loop frame.
//
// Who hits it: players with no mouse attached at all — which, for a mod whose
// entire premise is that the game is playable without one, is not an edge case.
//
// The guard returns the engine's own failure value when the interface is NULL.
// That is a state both callers already handle: the caller re-tests the device
// pointer and returns 0, and the mouse update compares against 1 and skips the
// frame's mouse handling. Nothing downstream sees a novel state.
//
// An inline trampoline, not a hooks.toml detour: the KPatchManager wrapper on
// this entry left the menu keyboard-dead until an alt-tab, most likely its
// PUSHAD/PUSHFD/CALL overhead colliding with DirectInput's cooperative-level
// handshake. A bare JMP has none of that.
const uintptr_t kAddrInitMouse =
    acc::addr::Pick(0x005e3fa0, 0x00731070);

// Where the trampoline rejoins the engine: the instruction after the prologue
// bytes we displace, plus (KOTOR 1 only) the interface load the trampoline
// re-executes itself.
const uintptr_t kAddrInitMouseContinue =
    acc::addr::Pick(0x005e3faa, 0x00731076);

// One past the last byte of InitializeDirectInputMouse — the scan bound for
// the teardown block below. KOTOR 1: Ghidra gives the function as
// 0x005e3fa0..0x005e4044 inclusive, with CExoRawInputInternal::GetMouseState
// (the per-frame update that lazily re-invokes the init) starting at
// 0x005e4050. KOTOR 2: the function's shared epilogue is reached by the
// JMP at the end of the CreateDevice-failed branch and lands at 0x007311b2,
// with the next function at 0x007311d0 — 0x007311c0 sits in the padding
// between them.
//
// Only the bound matters, not the exact last instruction: the scan rewrites a
// site only when its rel32 resolves to ShutDownDirectInput, and the nearest
// other caller of that function in either binary is the engine's own public
// shutdown wrapper, which sits BELOW the init's entry (KOTOR 2 0x00731051) and
// so is outside this range in both directions.
const uintptr_t kAddrInitMouseEnd =
    acc::addr::Pick(0x005e4045, 0x007311c0);

// CExoRawInputInternal::ShutDownDirectInput. The engine's real teardown: it
// Unacquires + Releases the keyboard device, then the mouse, then every
// joystick, frees the joystick arrays, Releases the DI8 interface and NULLs
// every one of those fields (same shape in both games).
const uintptr_t kAddrShutDownDirectInput =
    acc::addr::Pick(0x005e3dc0, 0x00733200);

// KOTOR 1's failure exit (POP EDI; XOR EAX,EAX; ...). KOTOR 1 needs it because
// its prologue has already pushed ESI/EDI by the time we test, so the fail path
// has a frame to unwind. KOTOR 2's function takes no stack arguments and ends
// in a bare RET, so its trampoline tests BEFORE building any frame and returns
// inline — there is no K2 address to find here, hence Kotor1Only.
const uintptr_t kAddrInitMouseFailEpilogue =
    acc::addr::Kotor1Only(0x005e401f);

// Entry bytes we displace, verified before we overwrite them. A mismatch means
// the address is wrong for the running build; skipping is the safe answer,
// since writing a JMP into the middle of an unrelated function corrupts state
// long before it faults.
const uint8_t kInitMousePrologueK1[] = {
    0x83, 0xec, 0x14,  // SUB ESP, 0x14
    0x56,              // PUSH ESI
    0x57,              // PUSH EDI
    0x8b, 0xf9,        // MOV EDI, ECX        (this)
};
const uint8_t kInitMousePrologueK2[] = {
    0x55,              // PUSH EBP
    0x8b, 0xec,        // MOV EBP, ESP
    0x83, 0xec, 0x30,  // SUB ESP, 0x30
};

// Set once a mouse device has proven un-creatable this session. A plain byte
// rather than a bool or an atomic because the mouse guard's trampoline tests it
// with a hand-written CMP against its absolute address, and because the only
// writer is the engine thread that owns DirectInput.
//
// Not a function-local static anywhere: the handler below runs under __try, and
// a function-local static's thread-safe-init guard is exactly the kind of
// unwinding MSVC refuses to mix with SEH (C2712).
volatile uint8_t g_mouseUnavailable = 0;

// One-shot log latches, for the same reason.
bool g_teardownLogged = false;

// Return addresses of the teardown call sites we rewrote, in address order,
// so the handler can say WHICH step of the mouse bring-up failed rather than
// just that one did. The order is the order of the steps in the function —
// CreateDevice, SetDataFormat, SetCooperativeLevel, SetProperty, Acquire —
// which the KOTOR 1 decompile spells out and KOTOR 2 recompiles unchanged.
// KOTOR 1's compiler folds the first four failures into one shared exit, so it
// has two sites where KOTOR 2 has five; the names below are indexed per game.
uintptr_t g_teardownSites[8] = {};
int       g_teardownSiteCount = 0;

const char* TeardownSiteName(int index, int total) {
    static const char* const k5[] = {
        "CreateDevice", "SetDataFormat", "SetCooperativeLevel", "SetProperty",
        "Acquire",
    };
    static const char* const k2sites[] = {
        "CreateDevice/SetDataFormat/SetCooperativeLevel/SetProperty (shared "
        "failure exit)",
        "Acquire",
    };
    if (index < 0) return "unknown step";
    if (total == 5 && index < 5) return k5[index];
    if (total == 2 && index < 2) return k2sites[index];
    return "unknown step";
}

// JMP rel32 from `at` to `target`.
void WriteJmpRel32(uint8_t* at, uintptr_t target) {
    at[0] = 0xe9;
    int32_t rel = static_cast<int32_t>(
        target - (reinterpret_cast<uintptr_t>(at) + 5));
    memcpy(at + 1, &rel, 4);
}

// Stand-in for CExoRawInputInternal::ShutDownDirectInput, reached only from the
// failure branches of InitializeDirectInputMouse (the engine's own shutdown
// path still calls the real thing). __fastcall with an unused second parameter
// is the exact stack contract of the __thiscall(void) it replaces: `this` in
// ECX, nothing pushed, nothing to clean.
//
// Doing nothing IS the fix. The branch that called us goes on to return the
// same value it always returned, and its caller — the per-frame mouse update —
// already treats that as "no mouse this frame". What no longer happens is the
// collateral: the keyboard device stays created and acquired.
void __fastcall MouseInitFailedInsteadOfTeardown(void* rawInput,
                                                 void* /*edx*/) {
    g_mouseUnavailable = 1;
    if (g_teardownLogged) return;
    g_teardownLogged = true;

    // Which failure branch called us. _ReturnAddress is the instruction after
    // the CALL we rewrote, which is exactly what the install loop recorded.
    const uintptr_t ret = reinterpret_cast<uintptr_t>(_ReturnAddress());
    int site = -1;
    for (int i = 0; i < g_teardownSiteCount; ++i) {
        if (g_teardownSites[i] == ret) { site = i; break; }
    }

    void* iface = nullptr;
    void* keyboard = nullptr;
    void* mouse = nullptr;
    __try {
        auto* base = static_cast<uint8_t*>(rawInput);
        iface    = *reinterpret_cast<void**>(base + raw_input::kOffInterface);
        keyboard = *reinterpret_cast<void**>(base + raw_input::kOffKeyboard);
        mouse    = *reinterpret_cast<void**>(base + raw_input::kOffMouse);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }

    acclog::Write("EngineInput",
        "MouseTeardownBlock: mouse bring-up failed at %s (site %d/%d, "
        "return=0x%08x); blocked the engine's ShutDownDirectInput "
        "(raw=%p interface=%p keyboard=%p mouse=%p). Vanilla would have "
        "released the KEYBOARD here and left it unrecoverable for the "
        "session; the mouse stays unavailable and the per-frame retry is now "
        "latched off",
        TeardownSiteName(site, g_teardownSiteCount), site + 1,
        g_teardownSiteCount, static_cast<unsigned>(ret),
        rawInput, iface, keyboard, mouse);
}

}  // namespace

void InstallDirectInputMouseGuard() {
    static bool installed = false;
    if (installed) return;
    installed = true;

    const bool k2 = acc::game::IsKotor2();

    if (!acc::addr::Ok(kAddrInitMouse) ||
        !acc::addr::Ok(kAddrInitMouseContinue) ||
        (!k2 && !acc::addr::Ok(kAddrInitMouseFailEpilogue))) {
        acclog::Write("EngineInput",
            "DirectInput mouse guard: no address for this build (%s); skipped",
            acc::game::TitleName());
        return;
    }

    auto* entry = reinterpret_cast<uint8_t*>(kAddrInitMouse);
    const uint8_t* expect = k2 ? kInitMousePrologueK2 : kInitMousePrologueK1;
    const size_t expectLen =
        k2 ? sizeof(kInitMousePrologueK2) : sizeof(kInitMousePrologueK1);
    if (memcmp(entry, expect, expectLen) != 0) {
        acclog::Write("EngineInput",
            "DirectInput mouse guard: entry bytes at %p are not the expected "
            "prologue (got %02x %02x %02x %02x %02x); skipped — the address is "
            "wrong for this build and patching it would corrupt .text",
            entry, entry[0], entry[1], entry[2], entry[3], entry[4]);
        return;
    }

    constexpr size_t kTrampolineSize = 64;
    auto* tramp = static_cast<uint8_t*>(VirtualAlloc(
        nullptr, kTrampolineSize, MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE));
    if (!tramp) {
        acclog::Write("EngineInput",
            "DirectInput mouse guard install failed: VirtualAlloc returned NULL");
        return;
    }

    uint8_t* p = tramp;

    // Mouse-absent latch, ahead of everything else.
    //
    // The engine's per-frame mouse update re-enters this function on EVERY
    // frame for as long as the mouse device pointer is NULL, so on a machine
    // where the device cannot be created the engine would otherwise run a
    // doomed CreateDevice (a COM call into DirectInput, device enumeration and
    // all) sixty times a second for the whole session. Once
    // InstallMouseTeardownBlock's handler has seen a failure there is nothing
    // left to try, so answer the engine's own failure value immediately.
    //
    // Placed before the interface test because it is the cheaper answer to the
    // same question and because it is valid in both games' register state: at
    // the entry nothing has been pushed, so XOR EAX,EAX / RET returns 0 with
    // the stack untouched (neither game's InitializeDirectInputMouse takes a
    // stack argument). The JNE's displacement is back-patched once the failure
    // stub's position is known.
    *p++ = 0x80; *p++ = 0x3d;                   // CMP byte ptr [g_mouseUnavailable], 0
    const uintptr_t flagAddr =
        reinterpret_cast<uintptr_t>(&g_mouseUnavailable);
    memcpy(p, &flagAddr, 4); p += 4;
    *p++ = 0x00;
    *p++ = 0x75;                                // JNE -> latch stub
    uint8_t* latchRel8 = p++;                   // back-patched below

    if (k2) {
        // Test first, before any frame exists: at the entry ECX is still `this`
        // and nothing has been pushed, so the failure path is a bare
        // XOR EAX,EAX / RET. The function takes no stack arguments (its sole
        // caller pushes nothing and it ends in a plain RET), so that returns 0
        // to the caller with the stack exactly as it found it.
        *p++ = 0x8b; *p++ = 0x41; *p++ = 0x20;  // MOV EAX, [ECX+0x20]
        *p++ = 0x85; *p++ = 0xc0;               // TEST EAX, EAX
        *p++ = 0x74; *p++ = 0x0b;               // JZ +11 -> the XOR below
        memcpy(p, kInitMousePrologueK2, sizeof(kInitMousePrologueK2));
        p += sizeof(kInitMousePrologueK2);
        WriteJmpRel32(p, kAddrInitMouseContinue); p += 5;
        // Failure stub. Shared with the latch above: both arrive with the
        // entry's register state intact, so both want this exact answer.
        *latchRel8 = static_cast<uint8_t>(p - (latchRel8 + 1));
        *p++ = 0x33; *p++ = 0xc0;               // XOR EAX, EAX
        *p++ = 0xc3;                            // RET
    } else {
        // KOTOR 1: run the displaced prologue (which puts `this` in EDI),
        // re-execute the interface load, then branch to the engine's own
        // continue point or its failure epilogue.
        memcpy(p, kInitMousePrologueK1, sizeof(kInitMousePrologueK1));
        p += sizeof(kInitMousePrologueK1);
        *p++ = 0x8b; *p++ = 0x47; *p++ = 0x1c;  // MOV EAX, [EDI+0x1c]
        *p++ = 0x85; *p++ = 0xc0;               // TEST EAX, EAX
        *p++ = 0x74; *p++ = 0x05;               // JZ +5 -> the fail JMP
        WriteJmpRel32(p, kAddrInitMouseContinue); p += 5;
        WriteJmpRel32(p, kAddrInitMouseFailEpilogue); p += 5;
        // The latch cannot share KOTOR 1's failure exit: that epilogue pops the
        // ESI/EDI the displaced prologue pushed, and the latch answers before
        // the prologue has run. Its own stub returns from the entry state.
        *latchRel8 = static_cast<uint8_t>(p - (latchRel8 + 1));
        *p++ = 0x33; *p++ = 0xc0;               // XOR EAX, EAX
        *p++ = 0xc3;                            // RET
    }

    DWORD oldProtect = 0;
    if (!VirtualProtect(entry, 5, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        acclog::Write("EngineInput",
            "DirectInput mouse guard install failed: VirtualProtect entry");
        return;
    }
    WriteJmpRel32(entry, reinterpret_cast<uintptr_t>(tramp));
    DWORD ignored = 0;
    VirtualProtect(entry, 5, oldProtect, &ignored);

    FlushInstructionCache(GetCurrentProcess(), entry, 5);
    FlushInstructionCache(GetCurrentProcess(), tramp, kTrampolineSize);

    acclog::Write("EngineInput",
        "DirectInput mouse guard installed (%s, InitializeDirectInputMouse "
        "@%p, trampoline @%p)", acc::game::TitleName(), entry, tramp);
}

void InstallMouseTeardownBlock() {
    static bool installed = false;
    if (installed) return;
    installed = true;

    if (!acc::addr::Ok(kAddrInitMouse) ||
        !acc::addr::Ok(kAddrInitMouseEnd) ||
        !acc::addr::Ok(kAddrShutDownDirectInput)) {
        acclog::Write("EngineInput",
            "MouseTeardownBlock: no address for this build (%s); skipped",
            acc::game::TitleName());
        return;
    }

    auto* lo = reinterpret_cast<uint8_t*>(kAddrInitMouse);
    auto* hi = reinterpret_cast<uint8_t*>(kAddrInitMouseEnd);
    if (hi <= lo) return;
    const size_t span = static_cast<size_t>(hi - lo);

    DWORD oldProtect = 0;
    if (!VirtualProtect(lo, span, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        acclog::Write("EngineInput",
            "MouseTeardownBlock: VirtualProtect over "
            "InitializeDirectInputMouse failed; skipped");
        return;
    }

    const uintptr_t replacement =
        reinterpret_cast<uintptr_t>(&MouseInitFailedInsteadOfTeardown);
    int patched = 0;
    for (size_t i = 0; i + 5 <= span; ++i) {
        if (lo[i] != 0xe8) continue;  // CALL rel32
        int32_t rel = 0;
        memcpy(&rel, lo + i + 1, 4);
        const uintptr_t after = reinterpret_cast<uintptr_t>(lo + i + 5);
        if (after + static_cast<uintptr_t>(static_cast<intptr_t>(rel)) !=
            kAddrShutDownDirectInput) {
            continue;
        }
        // Range check for form's sake: a rel32 spans +-2 GB, the image sits at
        // 0x00400000 and no user-mode module base reaches 0x80000000, so the
        // displacement from engine code to our DLL cannot overflow. Checked
        // anyway, because the failure mode if it ever did would be a call into
        // nowhere on the one path nobody exercises.
        const intptr_t delta =
            static_cast<intptr_t>(replacement) - static_cast<intptr_t>(after);
        if (delta > 0x7ffffff0 || delta < -0x7ffffff0) {
            acclog::Write("EngineInput",
                "MouseTeardownBlock: handler is out of rel32 range of %p; "
                "site left alone", lo + i);
            continue;
        }
        const int32_t newRel = static_cast<int32_t>(delta);
        memcpy(lo + i + 1, &newRel, 4);
        if (patched < static_cast<int>(sizeof(g_teardownSites) /
                                       sizeof(g_teardownSites[0]))) {
            g_teardownSites[patched] = after;  // the handler's return address
        }
        ++patched;
        i += 4;  // skip the displacement we just wrote
    }

    DWORD ignored = 0;
    VirtualProtect(lo, span, oldProtect, &ignored);
    FlushInstructionCache(GetCurrentProcess(), lo, span);

    g_teardownSiteCount = patched;

    if (patched == 0) {
        acclog::Write("EngineInput",
            "MouseTeardownBlock: found no ShutDownDirectInput call inside "
            "InitializeDirectInputMouse (%p..%p) — nothing patched. Either the "
            "addresses are wrong for this build or the engine changed shape; "
            "a mouse failure can still cost this session its keyboard",
            lo, hi);
        return;
    }
    acclog::Write("EngineInput",
        "MouseTeardownBlock: installed (%s) — %d teardown call site(s) inside "
        "InitializeDirectInputMouse now answer us instead of releasing the "
        "keyboard",
        acc::game::TitleName(), patched);
}

}  // namespace acc::engine

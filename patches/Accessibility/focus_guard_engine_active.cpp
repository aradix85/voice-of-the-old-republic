// The engine's own "render window active" flag, kept in step with the
// foreground (the maker's v0.9.3 fix, ported).
//
// DeactivateRenderWindow (KOTOR 1 0x00401d90, KOTOR 2 0x0040BC40) pauses the
// world, turns the client's input off and clears the flag;
// ActivateRenderWindow sets it again and undoes both. They run from the window
// procedure on WM_ACTIVATEAPP. KOTOR 2's step after an area load hides the
// loading screen only while the flag is set, and its movie-end handler
// activates the window only in full screen. In a window, a stray
// WM_ACTIVATEAPP(0) at a movie's end therefore leaves the loading screen up
// for good: nothing sends WM_ACTIVATEAPP(1) again while the game keeps the
// foreground. The maker's tester got out with Alt+Tab away and back, which is
// what the call below does.

#include "focus_guard.h"

#include <windows.h>

#include "bringup_announce.h"  // MovieRecentlyPlaying
#include "engine_addr.h"
#include "engine_reads.h"
#include "log.h"

namespace acc::focus_guard {

namespace {

// A .data dword. KOTOR 1: ActivateRenderWindow opens with
// MOV EAX,[0x007a3a38] / TEST / JNE ret and then stores 1 there. KOTOR 2: the
// decompile of 0x0040BCF0 reads `if (DAT_00a1b4e0 == 0) { DAT_00a1b4e0 = 1;`.
const uintptr_t kAddrRenderWindowActive =
    acc::addr::PickGlobal(0x007a3a38, 0x00a1b4e0);

// ActivateRenderWindow: no arguments, a plain RET in both games, and it does
// nothing unless the flag is 0. Sets the flag, lifts the focus-loss pause
// only when that pause was its own, turns the client's input on, restores
// sound mode and gamma.
typedef void (__cdecl* PFN_ActivateRenderWindow)();
const uintptr_t kAddrActivateRenderWindow =
    acc::addr::Pick(0x00401e00, 0x0040BCF0);

// The flag must have been 0 with the game in the foreground and no movie on
// screen (or ended this recently) for this long before the engine is told.
constexpr DWORD kMismatchMs   = 1500;
constexpr DWORD kMovieQuietMs = 2000;
constexpr DWORD kCooldownMs   = 5000;

DWORD g_mismatchSince = 0;  // 0 = no mismatch running
DWORD g_lastResyncAt  = 0;

bool CallActivateRenderWindow() {
    __try {
        reinterpret_cast<PFN_ActivateRenderWindow>(kAddrActivateRenderWindow)();
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

}  // namespace

void TickEngineActive() {
    int32_t flag = 0;
    const bool mismatch =
        acc::engine::TryReadI32(reinterpret_cast<void*>(kAddrRenderWindowActive),
                                0, &flag) &&
        flag == 0 && GameOwnsForeground() &&
        !acc::bringup_announce::MovieRecentlyPlaying(kMovieQuietMs);
    if (!mismatch) {
        g_mismatchSince = 0;
        return;
    }
    const DWORD now = GetTickCount();
    if (g_mismatchSince == 0) {
        g_mismatchSince = now ? now : 1;
        return;
    }
    if (now - g_mismatchSince < kMismatchMs) return;
    if (g_lastResyncAt != 0 && now - g_lastResyncAt < kCooldownMs) return;
    g_lastResyncAt = now ? now : 1;
    const DWORD heldMs = now - g_mismatchSince;
    g_mismatchSince = 0;
    const bool called = CallActivateRenderWindow();
    acclog::Write("EngineActive", "RESYNC: the engine thought its window "
                  "inactive for %lu ms with the game in the foreground and no "
                  "movie; ActivateRenderWindow %s",
                  static_cast<unsigned long>(heldMs),
                  called ? "called" : "FAULTED");
}

}  // namespace acc::focus_guard

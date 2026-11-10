// The movie-end foreground guard: after a cutscene movie the foreground can
// end up owned by no window at all, which leaves the engine paused for good;
// the guard reclaims the render window then, and only then.

#include "focus_guard_internal.h"

#include <windows.h>
#include <atomic>

#include "bringup_announce.h"
#include "log.h"

namespace acc::focus_guard {

namespace {

// Pull `target` to the foreground from a thread that doesn't own it. The
// AttachThreadInput dance is the standard way around the Win32 foreground
// lock without injecting a synthetic ALT keypress (which a screen-reader
// user would not want). SEH-guarded — every call here is a Win32 boundary.
void ForceForeground(HWND target, HWND fg) {
    __try {
        DWORD myTid = GetCurrentThreadId();
        DWORD fgTid = fg ? GetWindowThreadProcessId(fg, nullptr) : 0;
        BOOL attached = FALSE;
        if (fgTid && fgTid != myTid) {
            attached = AttachThreadInput(myTid, fgTid, TRUE);
        }
        BringWindowToTop(target);
        SetForegroundWindow(target);
        if (attached) {
            AttachThreadInput(myTid, fgTid, FALSE);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        // Reclaim is best-effort; swallow any Win32-boundary fault.
    }
}

}  // namespace

// --- Movie-end foreground guard (cutscene leaves the game with no foreground) ---
//
// The Peragus docking stunt (GOG K2, patch-20260905-075831) ended with the
// engine's movie player handing focus back, and then GetForegroundWindow()
// returning NULL — no window in the system owned the foreground. The engine
// auto-paused on the focus loss and never woke: from a blind player's chair the
// game said "Paused." and then answered nothing, indistinguishable from a
// crash. The vanilla GOG build is fragile to foreground loss generally; here
// the trigger was its own movie handoff.
//
// This guard arms for a short window each time a movie window disappears, and
// reclaims the render window ONLY when the foreground is genuinely orphaned
// (GetForegroundWindow() == NULL). That is the precise stuck state and nothing
// else:
//   * a foreign app holding the foreground means the user alt-tabbed away on
//     purpose — never trap them back (this was the original reason the startup
//     guard is bounded);
//   * our own window holding it (render live, or a movie segment still
//     transitioning) means nothing is wrong;
//   * a NEW movie segment starting inside the window means the cutscene resumed
//     — we skip, because forcing focus mid-playback can abort the engine's
//     movie queue and close the game (see bringup_announce IsMoviePlaying).
//
// Bounded hard: kMovieReclaimWindowMs total (a second or two, per the design
// ask) and kMovieReclaimMax attempts, so a persistently orphaned foreground
// cannot drag us into a focus war.
namespace {
constexpr int   kMovieReclaimMax      = 3;
}  // namespace

std::atomic<DWORD> g_movieReclaimDeadline{0};  // GetTickCount() deadline; 0 = disarmed
std::atomic<int>   g_movieReclaims{0};

void MaybeReclaimAfterMovie() {
    DWORD deadline = g_movieReclaimDeadline.load(std::memory_order_acquire);
    if (deadline == 0) return;  // disarmed

    DWORD now = GetTickCount();
    if (static_cast<int>(now - deadline) >= 0) {  // wrap-safe elapsed check
        g_movieReclaimDeadline.store(0, std::memory_order_release);
        acclog::Write("Focus",
            "MovieEndForegroundGuard: window elapsed; disarmed (reclaims=%d)",
            g_movieReclaims.load(std::memory_order_relaxed));
        return;
    }

    // A new movie segment resumed — let the movie player keep focus; forcing it
    // away mid-playback can abort the queue.
    if (acc::bringup_announce::IsMoviePlaying()) return;

    HWND render = g_liveRenderWindow.load(std::memory_order_acquire);
    if (!render || !IsWindow(render)) return;

    // Only act on a genuinely orphaned foreground. A foreign owner is a
    // deliberate alt-tab; our own owner means nothing is wrong.
    HWND fg = GetForegroundWindow();
    if (fg != nullptr) return;

    int n = g_movieReclaims.load(std::memory_order_relaxed);
    if (n >= kMovieReclaimMax) {
        g_movieReclaimDeadline.store(0, std::memory_order_release);
        acclog::Write("Focus",
            "MovieEndForegroundGuard: reclaim cap (%d) reached; giving up",
            kMovieReclaimMax);
        return;
    }
    g_movieReclaims.store(n + 1, std::memory_order_relaxed);

    acclog::Write("Focus",
        "MovieEndForegroundGuard: foreground orphaned (NULL) after movie; "
        "reclaiming render %p (attempt %d/%d)", render, n + 1, kMovieReclaimMax);
    ForceForeground(render, nullptr);
}

}  // namespace acc::focus_guard

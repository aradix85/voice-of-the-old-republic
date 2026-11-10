#include "focus_guard_internal.h"

#include <windows.h>
#include <atomic>
#include <cstdio>
#include <cstring>

#include "bringup_announce.h"
#include "engine_input.h"
#include "log.h"
#include "speech.h"
#include "strings.h"

namespace acc::focus_guard {

std::atomic<HWND> g_liveRenderWindow{nullptr};

namespace {

// KOTOR has multiple game-owned top-level windows (SWMovieWindow on the
// movie-player thread, "Render Window" on the main render thread, an
// invisible "Exo - BioWare Corp." legacy window). Focus can route to
// any of them, so we subclass all game windows simultaneously and tag
// every event with the source class name.
//
// Append-only table: the polling thread is the sole writer; SubclassProc
// is a hot-path reader called from per-message wndproc dispatch on
// arbitrary threads. Writers fully populate the entry's fields BEFORE
// publishing via the release-store of g_subclassedCount; readers do
// acquire-load on the count and then scan up to that many entries.
// Lock-free + correct on any sane memory model.
struct SubclassedWindow {
    HWND     hwnd;
    WNDPROC  origWndProc;
    char     tag[40];          // class name, for log disambiguation
};

// KOTOR creates fresh top-level windows on every phase transition
// (bootstrap → game, intro start/end, post-intro main menu). A clean
// startup with 3 intros consumes ~10 slots before main menu first-sight;
// stress-test alt-tab cycles add more. 64 buys us a comfortable headroom
// for a full session up to and beyond main menu without filling.
// Append-only — never reused — so the lock-free reader path stays safe.
constexpr int kMaxSubclassedWindows = 64;
SubclassedWindow         g_subclassed[kMaxSubclassedWindows] = {};
std::atomic<int>         g_subclassedCount{0};
std::atomic<HANDLE>      g_pollThread{nullptr};

const SubclassedWindow* FindSubclassed(HWND hwnd) {
    int n = g_subclassedCount.load(std::memory_order_acquire);
    for (int i = 0; i < n; ++i) {
        if (g_subclassed[i].hwnd == hwnd) return &g_subclassed[i];
    }
    return nullptr;
}

// On a focus-loss (WM_ACTIVATEAPP active=0) the foreground has moved to a
// window outside our process. Resolve and log which application took it —
// bare exe name, window class, and title — so user log bundles reveal the
// culprit (Steam/Discord/Game Bar overlays, a deliberate alt-tab target,
// etc.). Pure diagnostics: we neither announce nor reclaim here. Once we
// know the typical repeat offenders we can selectively warn for those.
// `activatingThread` is the WM_ACTIVATEAPP lParam (the thread being
// activated) logged alongside for cross-check.
void LogForegroundThief(DWORD activatingThread) {
    HWND fg = GetForegroundWindow();
    if (!fg) {
        acclog::Write("Focus",
            "ForegroundThief: GetForegroundWindow()=NULL "
            "(activatingThread=%lu)", activatingThread);
        return;
    }

    char cls[64] = {};
    char title[128] = {};
    GetClassNameA(fg, cls, sizeof(cls));
    GetWindowTextA(fg, title, sizeof(title));

    DWORD pid = 0;
    DWORD thread = GetWindowThreadProcessId(fg, &pid);

    char exe[MAX_PATH] = {};
    const char* exeName = "?";
    if (pid == GetCurrentProcessId()) {
        exeName = "(own process)";
    } else if (pid != 0) {
        HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (h) {
            DWORD sz = sizeof(exe);
            if (QueryFullProcessImageNameA(h, 0, exe, &sz)) {
                const char* slash = strrchr(exe, '\\');
                exeName = slash ? slash + 1 : exe;  // bare exe name
            }
            CloseHandle(h);
        }
    }

    acclog::Write("Focus",
        "ForegroundThief: hwnd=%p exe=\"%s\" class=\"%s\" title=\"%s\" "
        "pid=%lu thread=%lu activatingThread=%lu",
        fg, exeName, cls, title, pid, thread, activatingThread);
}

const char* ActivateName(WORD w) {
    switch (w) {
        case WA_INACTIVE:    return "WA_INACTIVE";
        case WA_ACTIVE:      return "WA_ACTIVE";
        case WA_CLICKACTIVE: return "WA_CLICKACTIVE";
        default:             return "?";
    }
}

LRESULT CALLBACK SubclassProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    const SubclassedWindow* entry = FindSubclassed(hwnd);
    const char* tag = entry ? entry->tag : "?";
    switch (msg) {
        case WM_ACTIVATE:
            acclog::Write("Focus",
                "WM_ACTIVATE %s minimized=%d otherHwnd=%p hwnd=%p [%s]",
                ActivateName(LOWORD(wp)),
                static_cast<int>(HIWORD(wp)),
                reinterpret_cast<void*>(lp), hwnd, tag);
            break;
        case WM_ACTIVATEAPP:
            acclog::Write("Focus",
                "WM_ACTIVATEAPP active=%d otherThread=%lu hwnd=%p [%s]",
                static_cast<int>(wp),
                static_cast<DWORD>(lp), hwnd, tag);
            // Focus regained. When KOTOR runs windowed and an external app
            // repeatedly steals the foreground, each regain forces the engine
            // to recreate its render window and the DirectInput `active` flag
            // can stick at 1 with the keyboard actually unacquired — input
            // goes dead, even in menus. Flag a reacquire for the next tick to
            // drive a real SetActive(0)->(1) edge (the software alt-tab). We
            // defer rather than call inline: this wndproc runs mid-pump inside
            // the engine's own activation handling, before the new render
            // window's DirectInput is fully rebound. See engine_input.h
            // RequestInputReacquire for the full mechanism.
            if (wp != 0) {
                acc::engine::RequestInputReacquire();
            } else {
                // Lost the foreground. Release the engine's DirectInput grab
                // next tick so background-level key reads stop bleeding into
                // the game while another window (a screen reader, any app) is
                // foreground. Input then mirrors foreground: held only while we
                // own it. Requested on every game-owned window's active=0
                // (coalesced); the thief log below is gated to the Render
                // Window so we emit one thief record per steal, not one per
                // window (they share a thread and all get WM_ACTIVATEAPP).
                acc::engine::RequestInputRelease();
                if (strcmp(tag, "Render Window") == 0) {
                    LogForegroundThief(static_cast<DWORD>(lp));
                }
            }
            break;
        case WM_SETFOCUS:
            acclog::Write("Focus",
                "WM_SETFOCUS prevHwnd=%p hwnd=%p [%s]",
                reinterpret_cast<void*>(wp), hwnd, tag);
            break;
        case WM_KILLFOCUS:
            acclog::Write("Focus",
                "WM_KILLFOCUS nextHwnd=%p hwnd=%p [%s]",
                reinterpret_cast<void*>(wp), hwnd, tag);
            break;
        case WM_SYSCOMMAND:
            // Kill the Alt menu. Alt is a SYSTEM key: DefWindowProc answers
            // Alt (alone or with a letter) by sending SC_KEYMENU to open the
            // window's menu bar. KOTOR has no menu bar, but the window still
            // enters the OS keyboard-menu mode, which swallows keyboard input
            // until Alt is pressed a second time — the user-visible symptom
            // was "Alt+R blocks input until I tap Alt again", with the log
            // going completely silent in between because the keys never
            // reached the engine at all.
            //
            // NOTE: this swallow alone did NOT clear that block — a live round
            // logged the swallow firing and the input still died, so Alt has
            // more going on here than menu mode (the engine's own WM_SYSKEYDOWN
            // / DirectInput handling is the remaining suspect, unexamined).
            // CopyLastSpoken moved to Ctrl+R rather than chase it. This stays
            // because it is correct on its own terms and PathfindFocusForce
            // (Alt+`-`) is still an Alt binding.
            //
            // This is above the engine, not inside it: the mod's own
            // modifier-shadow gates (input_pipeline / menus_dispatch) never
            // see these presses, so no amount of hotkey-registry work could
            // have prevented it. Swallowing SC_KEYMENU is the standard fix and
            // is what every fullscreen game does. SC_MOUSEMENU (clicking a
            // menu bar that does not exist) goes with it. Alt+F4 is SC_CLOSE
            // and is deliberately left alone.
            if ((wp & 0xFFF0) == SC_KEYMENU || (wp & 0xFFF0) == SC_MOUSEMENU) {
                acclog::Trace("Focus",
                    "SC_KEYMENU swallowed hwnd=%p [%s] — Alt must not open a "
                    "menu mode that eats keyboard input", hwnd, tag);
                return 0;
            }
            break;
        default:
            break;
    }
    if (entry && entry->origWndProc) {
        return CallWindowProcW(entry->origWndProc, hwnd, msg, wp, lp);
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// True for game-owned windows we want focus events from. KOTOR enumerated
// three: "SWMovieWindow" (movie player thread), "Render Window" (main
// render thread, where engine input pump lives), and "Exo - BioWare
// Corp., (c) 1999 - Generic Blank Application" (legacy invisible).
// Everything else from the PID's window list — MSCTFIME UI, IME, etc. —
// is OS-managed input infrastructure we don't care about.
bool IsGameWindowClass(const char* cls) {
    if (!cls || !*cls) return false;
    if (strcmp(cls, "MSCTFIME UI") == 0) return false;
    if (strcmp(cls, "IME") == 0) return false;
    // Never touch the Bink movie player's window. KOTOR's movie player
    // is fragile about window/focus churn during playback (cf. the
    // Alt+Tab-during-intros queue-restart bug); replacing its wndproc
    // cross-thread risks aborting the movie queue, which surfaces as the
    // game closing instead of starting the next queued movie. The phase
    // machine in bringup_announce already derives "movie playing" from
    // GetForegroundWindow without needing to subclass the window.
    if (strcmp(cls, "SWMovieWindow") == 0) return false;
    return true;
}

void LogOneWindow(HWND hwnd, const char* origin) {
    char cls[64] = {};
    char title[128] = {};
    GetClassNameA(hwnd, cls, sizeof(cls));
    GetWindowTextA(hwnd, title, sizeof(title));
    LONG style   = GetWindowLongA(hwnd, GWL_STYLE);
    LONG exStyle = GetWindowLongA(hwnd, GWL_EXSTYLE);
    HWND parent  = GetParent(hwnd);
    HWND owner   = GetWindow(hwnd, GW_OWNER);
    DWORD thread = GetWindowThreadProcessId(hwnd, nullptr);
    acclog::Write("Focus",
        "Windows[%s]: hwnd=%p class=\"%s\" title=\"%s\" "
        "style=0x%08lx exStyle=0x%08lx visible=%d parent=%p owner=%p "
        "thread=%lu",
        origin, hwnd, cls, title,
        static_cast<unsigned long>(style),
        static_cast<unsigned long>(exStyle),
        IsWindowVisible(hwnd) ? 1 : 0,
        parent, owner, thread);
}

struct EnumDiagState { DWORD pid; int count; };

BOOL CALLBACK EnumDiagProc(HWND hwnd, LPARAM lp) {
    auto* state = reinterpret_cast<EnumDiagState*>(lp);
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == state->pid) {
        LogOneWindow(hwnd, "top");
        ++state->count;
    }
    return TRUE;
}

void LogAllProcessWindows(const char* origin) {
    EnumDiagState state{ GetCurrentProcessId(), 0 };
    acclog::Write("Focus",
        "Windows[%s]: enumerating top-level windows for pid=%lu",
        origin, state.pid);
    EnumWindows(EnumDiagProc, reinterpret_cast<LPARAM>(&state));
    acclog::Write("Focus",
        "Windows[%s]: %d top-level window(s) in pid=%lu",
        origin, state.count, state.pid);
}

// Polling-thread scan: called every iteration. Single-writer; only this
// thread mutates g_subclassed[] / g_subclassedCount. Idempotent against
// already-subclassed HWNDs.
struct ScanState { DWORD pid; int newlySubclassed; HWND visibleRender; };

BOOL CALLBACK ScanProc(HWND hwnd, LPARAM lp) {
    auto* state = reinterpret_cast<ScanState*>(lp);
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid != state->pid) return TRUE;

    char cls[64] = {};
    GetClassNameA(hwnd, cls, sizeof(cls));
    if (!IsGameWindowClass(cls)) return TRUE;

    // Track the current live render window for the foreground guard. KOTOR
    // recreates the "Render Window" a few times during startup; only the
    // live one is visible, so filter on IsWindowVisible. Last writer in a
    // scan wins — in practice exactly one render window is visible.
    if (strcmp(cls, "Render Window") == 0 && IsWindowVisible(hwnd)) {
        state->visibleRender = hwnd;
    }

    int n = g_subclassedCount.load(std::memory_order_relaxed);
    for (int i = 0; i < n; ++i) {
        if (g_subclassed[i].hwnd == hwnd) return TRUE;  // already subclassed
    }
    if (n >= kMaxSubclassedWindows) {
        // Dedup per hwnd — without it the polling thread spams the log
        // every 100ms for every unsubclassed window, burying useful data.
        // acclog::Once emits the first occurrence only per unique tag.
        char dedup[64];
        _snprintf_s(dedup, sizeof(dedup), _TRUNCATE,
                    "FocusPoll.tablefull.%p", hwnd);
        acclog::Once(dedup,
            "FocusPoll: table full (cap=%d), missing hwnd=%p class=\"%s\"",
            kMaxSubclassedWindows, hwnd, cls);
        return TRUE;
    }

    // Populate the entry BEFORE publishing the count bump.
    SubclassedWindow& slot = g_subclassed[n];
    slot.hwnd = hwnd;
    strncpy_s(slot.tag, cls, _TRUNCATE);

    // Publish the fully populated entry BEFORE swapping the wndproc. From
    // the instant SetWindowLongPtr installs SubclassProc, the window's owner
    // thread dispatches messages into it concurrently. The old order (swap
    // first, publish origWndProc after) left a gap where SubclassProc found
    // no entry and fell through to DefWindowProc — the message was EATEN
    // instead of reaching the window's real proc. DirectInput's DIEmWin
    // worker window lost its exclusive-mouse-acquire handshake to exactly
    // that gap: the acquiring thread (engine input init, or our reacquire
    // cycle) then waits forever on a completion the swallowed message was
    // supposed to produce — the pre-main-menu KOTOR 2 startup hang of
    // 2026-08-25 (k2hang.dmp: main thread parked in dinput8's internal
    // wait, DIEmWin's thread idle in GetMessage). Only a FOREGROUND launch
    // arms it — a background Acquire fails fast with no handshake — which
    // is why every background test launch survived while the desktop-
    // shortcut launch hung three times in a row.
    //
    // A window always has a wndproc, so read it up front; SetWindowLongPtr
    // returns the same value unless another injector swapped procs in the
    // microseconds between (handled below).
    WNDPROC current = reinterpret_cast<WNDPROC>(
        GetWindowLongPtrW(hwnd, GWLP_WNDPROC));
    if (!current) {
        acclog::Write("Focus",
            "FocusPoll: GetWindowLongPtr failed hwnd=%p class=\"%s\" err=%lu",
            hwnd, cls, GetLastError());
        slot.hwnd = nullptr;  // unpublished — count stays unincremented
        return TRUE;
    }
    slot.origWndProc = current;
    // Release-store publishes the complete entry to readers.
    g_subclassedCount.store(n + 1, std::memory_order_release);

    LONG_PTR prev = SetWindowLongPtrW(
        hwnd, GWLP_WNDPROC,
        reinterpret_cast<LONG_PTR>(SubclassProc));
    if (!prev) {
        acclog::Write("Focus",
            "FocusPoll: SetWindowLongPtr failed hwnd=%p class=\"%s\" err=%lu",
            hwnd, cls, GetLastError());
        // The entry stays published but SubclassProc was never installed,
        // so it is never consulted for this hwnd; the slot is spent and the
        // window keeps its original proc. (The old code retried on the next
        // poll pass; this failure mode has never been observed in a log.)
        return TRUE;
    }
    if (reinterpret_cast<WNDPROC>(prev) != current) {
        // Someone re-subclassed between our read and our install: forward to
        // what we actually displaced. A dispatch racing this store forwards
        // once to `current` — a valid proc one step stale, which is still a
        // forward, never a swallow.
        slot.origWndProc = reinterpret_cast<WNDPROC>(prev);
    }
    ++state->newlySubclassed;
    acclog::Write("Focus",
        "FocusPoll: subclassed hwnd=%p class=\"%s\" origWndProc=%p",
        hwnd, cls, slot.origWndProc);
    return TRUE;
}

DWORD WINAPI PollProc(LPVOID) {
    acclog::Write("Focus",
        "FocusPoll: thread started (pid=%lu, tid=%lu); scanning every 100ms",
        GetCurrentProcessId(), GetCurrentThreadId());

    // First pass: log baseline window inventory so we have a snapshot
    // of "what existed when the polling thread woke up". After this,
    // only newly-subclassed windows get individual log lines.
    LogAllProcessWindows("poll_start");

    int  iter             = 0;
    bool moviePlayingPrev = false;  // poll-thread-local movie-window edge
    for (;;) {
        ScanState st{ GetCurrentProcessId(), 0, nullptr };
        EnumWindows(ScanProc, reinterpret_cast<LPARAM>(&st));
        if (st.visibleRender) {
            g_liveRenderWindow.store(st.visibleRender, std::memory_order_release);
        }

        // Arm the movie-end foreground guard on the true->false edge of movie
        // playback. Multi-segment cutscenes fire this per segment; each re-arms
        // the short window, and MaybeReclaimAfterMovie only acts once the
        // foreground is actually orphaned.
        bool moviePlayingNow = acc::bringup_announce::IsMoviePlaying();
        if (moviePlayingPrev && !moviePlayingNow) {
            g_movieReclaims.store(0, std::memory_order_relaxed);
            DWORD dl = GetTickCount() + kMovieReclaimWindowMs;
            if (dl == 0) dl = 1;  // 0 is the disarmed sentinel
            g_movieReclaimDeadline.store(dl, std::memory_order_release);
            acclog::Write("Focus",
                "MovieEndForegroundGuard: armed for %lu ms after movie end",
                kMovieReclaimWindowMs);
        }
        moviePlayingPrev = moviePlayingNow;

        MaybeReclaimAfterMovie();
        MaybeFlagInputBlockedWarning();
        Sleep(100);
        ++iter;
        // Heartbeat once per minute so we know the thread is still
        // alive (silent intervals during hangs are otherwise ambiguous).
        if (iter % 600 == 0) {
            acclog::Write("Focus",
                "FocusPoll: heartbeat iter=%d subclassed=%d",
                iter, g_subclassedCount.load(std::memory_order_relaxed));
        }
    }
}

}  // namespace

bool GameOwnsForeground() {
    HWND fg = GetForegroundWindow();
    if (!fg) return false;
    DWORD pid = 0;
    GetWindowThreadProcessId(fg, &pid);
    return pid == GetCurrentProcessId();
}

void StartFocusProbe() {
    HANDLE expected = nullptr;
    HANDLE created = CreateThread(
        nullptr, 0, PollProc, nullptr, CREATE_SUSPENDED, nullptr);
    if (!created) {
        acclog::Write("Focus",
            "StartFocusProbe: CreateThread failed err=%lu", GetLastError());
        return;
    }
    if (!g_pollThread.compare_exchange_strong(expected, created)) {
        // Lost the race — another caller already started the thread.
        CloseHandle(created);
        return;
    }
    ResumeThread(created);
    acclog::Write("Focus",
        "StartFocusProbe: polling thread launched (handle=%p)", created);
}

}  // namespace acc::focus_guard

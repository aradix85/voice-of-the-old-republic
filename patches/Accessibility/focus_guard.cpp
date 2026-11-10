// The focus probe: a poll thread that subclasses every game window to log
// its focus messages, feeds the input release/reacquire on focus loss and
// regain, swallows the Alt menu mode, and runs the movie-end guard and the
// Big Picture warning every 100 ms.

#include "focus_guard_internal.h"

#include <windows.h>
#include <atomic>
#include <cstdio>
#include <cstring>

#include "engine_input.h"
#include "log.h"

namespace acc::focus_guard {

namespace {

// The game has several top-level windows (SWMovieWindow on the movie thread,
// "Render Window" on the main thread, an invisible "Exo - BioWare Corp."
// window) and focus can land on any, so all of them are subclassed and every
// event carries the class name.
//
// The table is append-only and never reused: the poll thread is the only
// writer and fills an entry before the release-store of the count publishes
// it; SubclassProc reads on any window thread after an acquire-load. The
// game makes fresh windows at every phase change (about ten by the main menu
// with three intros), so 64 lasts a session.
struct SubclassedWindow {
    HWND     hwnd;
    WNDPROC  origWndProc;
    char     tag[40];  // class name
};

constexpr int kMaxSubclassedWindows = 64;
SubclassedWindow    g_subclassed[kMaxSubclassedWindows] = {};
std::atomic<int>    g_subclassedCount{0};
std::atomic<HANDLE> g_pollThread{nullptr};

const SubclassedWindow* FindSubclassed(HWND hwnd) {
    const int n = g_subclassedCount.load(std::memory_order_acquire);
    for (int i = 0; i < n; ++i) {
        if (g_subclassed[i].hwnd == hwnd) return &g_subclassed[i];
    }
    return nullptr;
}

// Which application took the foreground on a focus loss (Steam, Discord,
// Game Bar overlays, an alt-tab target). Diagnostics only.
void LogForegroundThief(DWORD activatingThread) {
    HWND fg = GetForegroundWindow();
    if (!fg) {
        acclog::Write("Focus", "ForegroundThief: GetForegroundWindow()=NULL "
                      "(activatingThread=%lu)", activatingThread);
        return;
    }
    char cls[64] = {};
    char title[128] = {};
    GetClassNameA(fg, cls, sizeof(cls));
    GetWindowTextA(fg, title, sizeof(title));
    DWORD pid = 0;
    const DWORD thread = GetWindowThreadProcessId(fg, &pid);

    char exe[MAX_PATH] = {};
    const char* exeName = "?";
    if (pid == GetCurrentProcessId()) {
        exeName = "(own process)";
    } else if (const char* name = pid ? ProcessExeName(pid, exe, sizeof(exe)) : nullptr) {
        exeName = name;
    }
    acclog::Write("Focus", "ForegroundThief: hwnd=%p exe=\"%s\" class=\"%s\" title=\"%s\" "
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
            acclog::Write("Focus", "WM_ACTIVATE %s minimized=%d otherHwnd=%p hwnd=%p [%s]",
                          ActivateName(LOWORD(wp)), static_cast<int>(HIWORD(wp)),
                          reinterpret_cast<void*>(lp), hwnd, tag);
            break;
        case WM_ACTIVATEAPP:
            acclog::Write("Focus", "WM_ACTIVATEAPP active=%d otherThread=%lu hwnd=%p [%s]",
                          static_cast<int>(wp), static_cast<DWORD>(lp), hwnd, tag);
            if (wp != 0) {
                // A regain can leave DirectInput's `active` flag at 1 with the
                // keyboard unacquired; the reacquire runs next tick, once the
                // new render window's DirectInput is rebound.
                acc::engine::RequestInputReacquire();
            } else {
                // Input mirrors the foreground: released while another
                // window has it. Every game window gets this message, so the
                // thief is logged once, for the Render Window.
                acc::engine::RequestInputRelease();
                if (strcmp(tag, "Render Window") == 0) {
                    LogForegroundThief(static_cast<DWORD>(lp));
                }
            }
            break;
        case WM_SETFOCUS:
            acclog::Write("Focus", "WM_SETFOCUS prevHwnd=%p hwnd=%p [%s]",
                          reinterpret_cast<void*>(wp), hwnd, tag);
            break;
        case WM_KILLFOCUS:
            acclog::Write("Focus", "WM_KILLFOCUS nextHwnd=%p hwnd=%p [%s]",
                          reinterpret_cast<void*>(wp), hwnd, tag);
            break;
        case WM_SYSCOMMAND:
            // Alt answers with SC_KEYMENU, and the menu-bar-less window then
            // enters keyboard-menu mode, which eats keys until Alt is pressed
            // again; this happens above the engine, out of reach of the
            // hotkey gates. Alt+letter still blocked input with this in
            // place (an unexamined engine-side cause), which is why no
            // binding uses Alt+letter. Alt+F4 (SC_CLOSE) passes.
            if ((wp & 0xFFF0) == SC_KEYMENU || (wp & 0xFFF0) == SC_MOUSEMENU) {
                acclog::Trace("Focus", "SC_KEYMENU swallowed hwnd=%p [%s]", hwnd, tag);
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

// The game windows worth subclassing: not the OS input windows, and never
// the Bink movie window, whose player aborts its queue (closing the game) on
// a cross-thread wndproc swap mid-playback.
bool IsGameWindowClass(const char* cls) {
    return cls && *cls && strcmp(cls, "MSCTFIME UI") != 0 && strcmp(cls, "IME") != 0 &&
           strcmp(cls, "SWMovieWindow") != 0;
}

struct EnumDiagState { DWORD pid; int count; };

BOOL CALLBACK EnumDiagProc(HWND hwnd, LPARAM lp) {
    auto* state = reinterpret_cast<EnumDiagState*>(lp);
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid != state->pid) return TRUE;
    char cls[64] = {};
    char title[128] = {};
    GetClassNameA(hwnd, cls, sizeof(cls));
    GetWindowTextA(hwnd, title, sizeof(title));
    acclog::Write("Focus", "Windows[poll_start]: hwnd=%p class=\"%s\" title=\"%s\" "
                  "style=0x%08lx exStyle=0x%08lx visible=%d parent=%p owner=%p thread=%lu",
                  hwnd, cls, title,
                  static_cast<unsigned long>(GetWindowLongA(hwnd, GWL_STYLE)),
                  static_cast<unsigned long>(GetWindowLongA(hwnd, GWL_EXSTYLE)),
                  IsWindowVisible(hwnd) ? 1 : 0, GetParent(hwnd), GetWindow(hwnd, GW_OWNER),
                  GetWindowThreadProcessId(hwnd, nullptr));
    ++state->count;
    return TRUE;
}

// The baseline window inventory when the poll thread starts; afterwards only
// newly subclassed windows are logged.
void LogAllProcessWindows() {
    EnumDiagState state{GetCurrentProcessId(), 0};
    EnumWindows(EnumDiagProc, reinterpret_cast<LPARAM>(&state));
    acclog::Write("Focus", "Windows[poll_start]: %d top-level window(s) in pid=%lu",
                  state.count, state.pid);
}

struct ScanState { DWORD pid; int newlySubclassed; HWND visibleRender; };

// One window of the poll thread's scan (the table's only writer).
BOOL CALLBACK ScanProc(HWND hwnd, LPARAM lp) {
    auto* state = reinterpret_cast<ScanState*>(lp);
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid != state->pid) return TRUE;
    char cls[64] = {};
    GetClassNameA(hwnd, cls, sizeof(cls));
    if (!IsGameWindowClass(cls)) return TRUE;

    // The game recreates its Render Window during startup; only the live one
    // is visible.
    if (strcmp(cls, "Render Window") == 0 && IsWindowVisible(hwnd)) {
        state->visibleRender = hwnd;
    }

    const int n = g_subclassedCount.load(std::memory_order_relaxed);
    for (int i = 0; i < n; ++i) {
        if (g_subclassed[i].hwnd == hwnd) return TRUE;
    }
    if (n >= kMaxSubclassedWindows) {
        char dedup[64];
        _snprintf_s(dedup, sizeof(dedup), _TRUNCATE, "FocusPoll.tablefull.%p", hwnd);
        acclog::Once(dedup, "FocusPoll: table full (cap=%d), missing hwnd=%p class=\"%s\"",
                     kMaxSubclassedWindows, hwnd, cls);
        return TRUE;
    }

    // Publish the complete entry BEFORE installing SubclassProc: from that
    // instant the window's thread dispatches into it, and a message that
    // finds no entry falls through to DefWindowProc and is lost. DirectInput's
    // DIEmWin window losing its exclusive-acquire handshake that way hung a
    // foreground K2 launch before the main menu.
    SubclassedWindow& slot = g_subclassed[n];
    slot.hwnd = hwnd;
    strncpy_s(slot.tag, cls, _TRUNCATE);
    auto current = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(hwnd, GWLP_WNDPROC));
    if (!current) {
        acclog::Write("Focus", "FocusPoll: GetWindowLongPtr failed hwnd=%p class=\"%s\" err=%lu",
                      hwnd, cls, GetLastError());
        slot.hwnd = nullptr;  // not published
        return TRUE;
    }
    slot.origWndProc = current;
    g_subclassedCount.store(n + 1, std::memory_order_release);

    const LONG_PTR prev =
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(SubclassProc));
    if (!prev) {
        // The slot is spent; the window keeps its own proc.
        acclog::Write("Focus", "FocusPoll: SetWindowLongPtr failed hwnd=%p class=\"%s\" err=%lu",
                      hwnd, cls, GetLastError());
        return TRUE;
    }
    // Another injector swapped procs in between: forward to what was really
    // displaced (a racing dispatch forwards once to `current`, still a proc).
    if (reinterpret_cast<WNDPROC>(prev) != current) {
        slot.origWndProc = reinterpret_cast<WNDPROC>(prev);
    }
    ++state->newlySubclassed;
    acclog::Write("Focus", "FocusPoll: subclassed hwnd=%p class=\"%s\" origWndProc=%p",
                  hwnd, cls, slot.origWndProc);
    return TRUE;
}

DWORD WINAPI PollProc(LPVOID) {
    acclog::Write("Focus", "FocusPoll: thread started (pid=%lu, tid=%lu); scanning every 100ms",
                  GetCurrentProcessId(), GetCurrentThreadId());
    LogAllProcessWindows();

    HWND liveRender = nullptr;
    for (int iter = 1;; ++iter) {
        ScanState st{GetCurrentProcessId(), 0, nullptr};
        EnumWindows(ScanProc, reinterpret_cast<LPARAM>(&st));
        if (st.visibleRender) liveRender = st.visibleRender;

        TickMovieEndGuard(liveRender);
        MaybeFlagInputBlockedWarning();
        Sleep(100);
        if (iter % 600 == 0) {  // once a minute: the thread is alive
            acclog::Write("Focus", "FocusPoll: heartbeat iter=%d subclassed=%d",
                          iter, g_subclassedCount.load(std::memory_order_relaxed));
        }
    }
}

}  // namespace

const char* ProcessExeName(DWORD pid, char* buf, DWORD bufSize) {
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return nullptr;
    DWORD size = bufSize;
    const BOOL ok = QueryFullProcessImageNameA(h, 0, buf, &size);
    CloseHandle(h);
    if (!ok) return nullptr;
    const char* slash = strrchr(buf, '\\');
    return slash ? slash + 1 : buf;
}

bool GameOwnsForeground() {
    HWND fg = GetForegroundWindow();
    if (!fg) return false;
    DWORD pid = 0;
    GetWindowThreadProcessId(fg, &pid);
    return pid == GetCurrentProcessId();
}

void StartFocusProbe() {
    HANDLE created = CreateThread(nullptr, 0, PollProc, nullptr, CREATE_SUSPENDED, nullptr);
    if (!created) {
        acclog::Write("Focus", "StartFocusProbe: CreateThread failed err=%lu", GetLastError());
        return;
    }
    HANDLE expected = nullptr;
    if (!g_pollThread.compare_exchange_strong(expected, created)) {
        CloseHandle(created);  // already running
        return;
    }
    ResumeThread(created);
    acclog::Write("Focus", "StartFocusProbe: polling thread launched (handle=%p)", created);
}

}  // namespace acc::focus_guard

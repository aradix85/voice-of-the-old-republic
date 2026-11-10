// The input-blocked warning: keys pressed while Steam Big Picture holds the
// foreground never reach the game, so the poll thread notices the keypress
// and the main tick speaks a warning.

#include "focus_guard_internal.h"

#include <windows.h>
#include <atomic>
#include <cstring>

#include "log.h"
#include "speech.h"
#include "strings.h"

namespace acc::focus_guard {

namespace {

// --- Input-blocked warning: Steam Big Picture foreground theft ---
//
// When KOTOR runs windowed and Steam Big Picture Mode holds the foreground,
// the user's keystrokes route to Big Picture, not the game — our engine input
// hook never sees them, so menus look dead (zero Menus.Input events across
// kenny's patch-20260612-150316.log while Big Picture repeatedly stole focus).
// Because the keypress never reaches the engine, the poll thread samples
// GetAsyncKeyState directly and raises a one-shot spoken warning on the next
// tick. We deliberately do NOT reclaim the foreground here — that would trap a
// user who actually wants Big Picture (cf. the bounded startup guard above).
//
// Throttle: at most one warning per kWarnCooldownMs, and only on a fresh key
// edge. Focus flickers sub-second under Big Picture, so a per-steal warning
// would spam; gating on an actual keypress means we only speak when the user
// is genuinely trying to interact.
constexpr DWORD kWarnCooldownMs = 20000;

std::atomic<bool>  g_warnPending{false};   // poll thread -> main tick drain
std::atomic<DWORD> g_lastWarnTick{0};      // GetTickCount of last warn; 0=never
bool               g_interactionKeyDownPrev = false;  // poll-thread only

// Case-insensitive substring test (ASCII). Avoids a shlwapi dependency for
// the one "Big Picture" title match below.
bool ContainsNoCase(const char* hay, const char* needle) {
    if (!hay || !needle || !*needle) return false;
    for (; *hay; ++hay) {
        const char* h = hay;
        const char* n = needle;
        while (*h && *n && ((*h | 0x20) == (*n | 0x20))) { ++h; ++n; }
        if (!*n) return true;
    }
    return false;
}

// True while any "the user is trying to interact" key is physically down:
// arrows / Enter / Space / Esc / Tab / Backspace / digits / letters / F-keys.
// Pure modifiers (Shift/Ctrl/Alt/Caps) are excluded so a held modifier alone
// doesn't read as intent. GetAsyncKeyState reports physical state regardless
// of focus and needs no message loop — exactly right for an unfocused window.
bool AnyInteractionKeyDown() {
    static const int kKeys[] = {
        VK_RETURN, VK_SPACE, VK_ESCAPE, VK_TAB, VK_BACK,
        VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN,
    };
    for (int vk : kKeys) {
        if (GetAsyncKeyState(vk) & 0x8000) return true;
    }
    for (int vk = '0'; vk <= '9'; ++vk) {
        if (GetAsyncKeyState(vk) & 0x8000) return true;
    }
    for (int vk = 'A'; vk <= 'Z'; ++vk) {
        if (GetAsyncKeyState(vk) & 0x8000) return true;
    }
    for (int vk = VK_F1; vk <= VK_F12; ++vk) {
        if (GetAsyncKeyState(vk) & 0x8000) return true;
    }
    return false;
}

// Is the current foreground window Steam Big Picture? steamwebhelper.exe also
// backs the normal Steam client UI, so we additionally require the window
// title to name Big Picture. (kenny is en-US; broaden if a localized title
// ever surfaces.) False for our own process and every other app.
bool ForegroundIsBigPicture() {
    HWND fg = GetForegroundWindow();
    if (!fg) return false;
    DWORD pid = 0;
    GetWindowThreadProcessId(fg, &pid);
    if (pid == 0 || pid == GetCurrentProcessId()) return false;

    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return false;
    char exe[MAX_PATH] = {};
    DWORD sz = sizeof(exe);
    bool isSteamHelper = false;
    if (QueryFullProcessImageNameA(h, 0, exe, &sz)) {
        const char* slash = strrchr(exe, '\\');
        const char* name = slash ? slash + 1 : exe;
        isSteamHelper = _stricmp(name, "steamwebhelper.exe") == 0;
    }
    CloseHandle(h);
    if (!isSteamHelper) return false;

    char title[128] = {};
    GetWindowTextA(fg, title, sizeof(title));
    return ContainsNoCase(title, "Big Picture");
}

}  // namespace

// Poll-thread step: if the user pressed an interaction key while Steam Big
// Picture holds the foreground, queue a spoken warning for the main tick to
// drain. Edge- and cooldown-gated (see kWarnCooldownMs).
void MaybeFlagInputBlockedWarning() {
    bool keyDown = AnyInteractionKeyDown();
    bool risingEdge = keyDown && !g_interactionKeyDownPrev;
    g_interactionKeyDownPrev = keyDown;
    if (!risingEdge) return;
    if (!ForegroundIsBigPicture()) return;

    DWORD now = GetTickCount();
    DWORD last = g_lastWarnTick.load(std::memory_order_relaxed);
    if (last != 0 && (now - last) < kWarnCooldownMs) return;
    g_lastWarnTick.store(now ? now : 1, std::memory_order_relaxed);

    g_warnPending.store(true, std::memory_order_release);
    acclog::Write("Focus",
        "InputBlockedWarning: Steam Big Picture holds foreground and an "
        "interaction key was pressed; queuing spoken warning (cooldown %lums)",
        kWarnCooldownMs);
}

void DrainInputBlockedWarning() {
    if (!g_warnPending.exchange(false, std::memory_order_acquire)) return;
    acclog::Write("Focus",
        "InputBlockedWarning: speaking Big Picture focus-theft warning");
    // Spoken on the main tick (COM-safe), not the poll thread.
    acc::speech::Speak(acc::strings::Get(acc::strings::Id::InputBlockedBigPicture), /*interrupt=*/true);
}

}  // namespace acc::focus_guard

// The input-blocked warning. When the game runs windowed and Steam Big
// Picture holds the foreground, keystrokes go to Big Picture and never reach
// the engine, so menus look dead. The poll thread samples the physical keys
// and flags a warning that the main tick speaks. The foreground is never
// reclaimed here: that would trap a player who wants Big Picture.

#include "focus_guard_internal.h"

#include <windows.h>
#include <atomic>
#include <cstring>

#include "log.h"
#include "speech.h"
#include "strfmt.h"
#include "strings.h"

namespace acc::focus_guard {

namespace {

// Focus flickers sub-second under Big Picture, so the warning waits for a
// fresh key edge and then holds off this long.
constexpr DWORD kWarnCooldownMs = 20000;

std::atomic<bool> g_warnPending{false};  // poll thread -> main tick
DWORD g_lastWarnTick = 0;                // poll thread only; 0 = never
bool  g_interactionKeyDownPrev = false;  // poll thread only

// Any key that means "trying to interact" is physically down (modifiers
// alone do not count). GetAsyncKeyState needs neither focus nor a message
// loop.
bool AnyInteractionKeyDown() {
    static const int kKeys[] = {VK_RETURN, VK_SPACE, VK_ESCAPE, VK_TAB, VK_BACK,
                                VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN};
    auto down = [](int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; };
    for (int vk : kKeys) if (down(vk)) return true;
    for (int vk = '0'; vk <= '9'; ++vk) if (down(vk)) return true;
    for (int vk = 'A'; vk <= 'Z'; ++vk) if (down(vk)) return true;
    for (int vk = VK_F1; vk <= VK_F12; ++vk) if (down(vk)) return true;
    return false;
}

// steamwebhelper.exe also backs the normal Steam client, so the title must
// name Big Picture too.
bool ForegroundIsBigPicture() {
    HWND fg = GetForegroundWindow();
    if (!fg) return false;
    DWORD pid = 0;
    GetWindowThreadProcessId(fg, &pid);
    if (pid == 0 || pid == GetCurrentProcessId()) return false;
    char exe[MAX_PATH] = {};
    const char* name = ProcessExeName(pid, exe, sizeof(exe));
    if (!name || _stricmp(name, "steamwebhelper.exe") != 0) return false;
    char title[128] = {};
    GetWindowTextA(fg, title, sizeof(title));
    return acc::strfmt::ContainsNoCase(title, "Big Picture");
}

}  // namespace

void MaybeFlagInputBlockedWarning() {
    const bool keyDown = AnyInteractionKeyDown();
    const bool risingEdge = keyDown && !g_interactionKeyDownPrev;
    g_interactionKeyDownPrev = keyDown;
    if (!risingEdge || !ForegroundIsBigPicture()) return;

    const DWORD now = GetTickCount();
    if (g_lastWarnTick != 0 && now - g_lastWarnTick < kWarnCooldownMs) return;
    g_lastWarnTick = now ? now : 1;
    g_warnPending.store(true, std::memory_order_release);
    acclog::Write("Focus", "InputBlockedWarning: Steam Big Picture holds foreground and an "
                  "interaction key was pressed; queuing spoken warning (cooldown %lums)",
                  kWarnCooldownMs);
}

// On the main tick, where speech is COM-safe.
void DrainInputBlockedWarning() {
    if (!g_warnPending.exchange(false, std::memory_order_acquire)) return;
    acclog::Write("Focus", "InputBlockedWarning: speaking Big Picture focus-theft warning");
    acc::speech::Speak(acc::strings::Get(acc::strings::Id::InputBlockedBigPicture),
                       /*interrupt=*/true);
}

}  // namespace acc::focus_guard

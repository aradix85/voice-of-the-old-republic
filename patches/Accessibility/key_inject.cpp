#include "key_inject.h"

#include <windows.h>

#pragma comment(lib, "user32.lib")

namespace acc::key_inject {

void Send(int scancode, bool down, bool extended) {
    if (scancode == 0) return;
    INPUT inp = {};
    inp.type       = INPUT_KEYBOARD;
    inp.ki.wScan   = static_cast<WORD>(scancode & 0xFF);
    inp.ki.dwFlags = KEYEVENTF_SCANCODE;
    if (extended) inp.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    if (!down)    inp.ki.dwFlags |= KEYEVENTF_KEYUP;
    SendInput(1, &inp, sizeof(INPUT));
}

void Tap(int scancode, bool extended) {
    Send(scancode, /*down=*/true,  extended);
    Send(scancode, /*down=*/false, extended);
}

namespace {

constexpr DWORD kHeldTapMs = 80;

int   g_heldScan     = 0;
bool  g_heldExtended = false;
DWORD g_heldSince    = 0;

void ReleaseHeld() {
    if (g_heldScan == 0) return;
    Send(g_heldScan, /*down=*/false, g_heldExtended);
    g_heldScan = 0;
}

}  // namespace

void TapHeld(int scancode, bool extended) {
    ReleaseHeld();
    // DirectInput's code for a key behind the E0 prefix is its base code
    // with the high bit set.
    if (scancode & 0x80) {
        scancode &= 0x7F;
        extended = true;
    }
    if (scancode == 0) return;
    Send(scancode, /*down=*/true, extended);
    g_heldScan     = scancode;
    g_heldExtended = extended;
    g_heldSince    = GetTickCount();
}

void Tick() {
    if (g_heldScan != 0 && GetTickCount() - g_heldSince >= kHeldTapMs) ReleaseHeld();
}

}  // namespace acc::key_inject

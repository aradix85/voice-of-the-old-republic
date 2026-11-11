// The hotkey registry: the live bindings and the per-tick edge detection.
// See hotkeys.h for the API contract.

#include "hotkeys_internal.h"
#include "hotkeys_chord_latch.h"

#include <windows.h>

#pragma comment(lib, "user32.lib")

namespace acc::hotkeys {

Binding g_bindings[static_cast<int>(Action::COUNT)] = {};
Binding g_defaults[static_cast<int>(Action::COUNT)] = {};

namespace {

// Per action, parallel to g_bindings. `now` is sampled in BeginTick and moves
// to `last` in EndTick; a press is `now && !last`.
struct EdgeState {
    bool now;
    bool last;
    // The binding's key alone, whatever the modifiers. Two actions can share a
    // key and differ only in modifiers (Ctrl+R copies, bare R repeats);
    // releasing the modifier while the key is still down must not hand the
    // bare action a press, so a press also needs the key up last tick.
    bool keyNow;
    bool keyLast;
    // Set by ClaimRisingEdge between EndTick and BeginTick (the engine's input
    // dispatch), where Consume cannot reach the coming edge; cleared in EndTick.
    bool claimed;
};
EdgeState g_edge[static_cast<int>(Action::COUNT)] = {};

ChordLatch g_chordLatch;

bool AnyDown(int a, int b, int c) {
    return KeyDown(a) || KeyDown(b) || KeyDown(c);
}

// AltGr (right Alt) counts as Alt too, but has its own bit for the bindings
// that tell the two apart.
uint32_t ReadModifiers() {
    uint32_t m = 0;
    if (ShiftHeld()) m |= kModShift;
    if (CtrlHeld()) m |= kModCtrl;
    if (AnyDown(VK_MENU, VK_LMENU, VK_RMENU)) m |= kModAlt;
    if (KeyDown(VK_RMENU)) m |= kModAltGr;
    return m;
}

bool BindingKeyDown(const Binding& b) {
    return KeyDown(b.vk) || KeyDown(b.altVk);
}

// The index of `a` once the defaults exist, or -1 when out of range.
int Index(Action a) {
    InitDefaults();
    const int idx = static_cast<int>(a);
    return (idx >= 0 && idx < static_cast<int>(Action::COUNT)) ? idx : -1;
}

}  // namespace

void BeginTick() {
    InitDefaults();
    EnsureOverridesLoaded();
    const uint32_t mods = ReadModifiers();
    const uint64_t nowMs = GetTickCount64();
    for (int i = 0; i < static_cast<int>(Action::COUNT); ++i) {
        const Binding& b = g_bindings[i];
        EdgeState& e = g_edge[i];
        e.keyNow = BindingKeyDown(b);
        e.now    = e.keyNow && ModsSatisfy(b, mods);
        if (b.modsRequired != 0 && e.now && !e.last && !e.keyLast) {
            g_chordLatch.ChordDown(KeyDown(b.vk) ? b.vk : b.altVk, nowMs);
        }
    }
}

void ChordPressSwallowed(int vk) {
    g_chordLatch.EngineSwallowed(vk, GetTickCount64());
}

bool TakeChordPress(int vk) {
    return g_chordLatch.TakeEnginePress(vk, GetTickCount64());
}

void EndTick() {
    for (EdgeState& e : g_edge) {
        e.last    = e.now;
        e.keyLast = e.keyNow;
        e.claimed = false;
    }
}

bool IsForegroundGame() {
    HWND fg = GetForegroundWindow();
    if (!fg) return false;
    DWORD pid = 0;
    GetWindowThreadProcessId(fg, &pid);
    return pid == GetCurrentProcessId();
}

bool Pressed(Action a) {
    const int i = Index(a);
    if (i < 0) return false;
    const EdgeState& e = g_edge[i];
    if (e.claimed || !e.now || e.last || e.keyLast) return false;
    return IsForegroundGame();
}

bool Held(Action a) {
    const int i = Index(a);
    return i >= 0 && g_edge[i].now;
}

// Pressed() stays false for the rest of this tick.
void Consume(Action a) {
    const int i = Index(a);
    if (i >= 0) g_edge[i].last = g_edge[i].now;
}

void ClaimRisingEdge(Action a) {
    const int i = Index(a);
    if (i >= 0) g_edge[i].claimed = true;
}

bool KeyDown(int vk) {
    return vk != 0 && (GetAsyncKeyState(vk) & 0x8000) != 0;
}

bool ShiftHeld() {
    return AnyDown(VK_SHIFT, VK_LSHIFT, VK_RSHIFT);
}

bool CtrlHeld() {
    return AnyDown(VK_CONTROL, VK_LCONTROL, VK_RCONTROL);
}

uint32_t CurrentModifiers() {
    return ReadModifiers();
}

bool ModifiedComboOwns(int vk) {
    if (vk == 0) return false;
    InitDefaults();
    const uint32_t mods = ReadModifiers();
    if ((mods & kAnyMod) == 0) return false;  // bare keys are the engine's
    // No check that `vk` is still down: the engine event is the proof of the
    // press, and a quick tap can be up again before a buffered engine command
    // arrives (Shift+L), while the modifier is still held.
    for (const Binding& b : g_bindings) {
        if (b.modsRequired != 0 && UsesKey(b, vk) && ModsSatisfy(b, mods)) return true;
    }
    return false;
}

Binding Get(Action a) {
    const int i = Index(a);
    return i >= 0 ? g_bindings[i] : Binding{};
}

void Set(Action a, Binding b) {
    const int i = Index(a);
    if (i < 0) return;
    g_bindings[i] = b;
    // A new binding must not fire on keys held at the moment of the rebind.
    g_edge[i].now     = false;
    g_edge[i].last    = false;
    g_edge[i].claimed = false;
}

}  // namespace acc::hotkeys

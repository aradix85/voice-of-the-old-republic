// central hotkey registry implementation.
// See hotkeys.h for the design rationale + API contract.

#include "hotkeys_internal.h"

namespace acc::hotkeys {

namespace {

// Per-Action edge-detection state. Stored in arrays parallel to `g_bindings`
// so an Action's index lookup is a single subscript. `now` is sampled in
// BeginTick; `last` is shifted from `now` in EndTick. `Pressed` returns
// `now && !last` (plus foreground gate).
//
// `claimed` is a one-shot guard set by ClaimRisingEdge from sites that
// fire between EndTick and BeginTick (the engine's input-dispatch window),
// where Consume() can't suppress the upcoming rising edge because
// BeginTick hasn't run yet. EndTick clears the guard so the next genuine
// rising edge fires normally.
struct EdgeState {
    bool now;
    bool last;
    // Raw physical key state, modifier-blind, for the same binding. Needed
    // because two Actions can share one key and differ only in modifiers
    // (Alt+R copies the last spoken text, bare R repeats the dialog line).
    // Releasing the modifier while the key is still held used to hand the
    // bare Action a rising edge it never earned: its `now` was false while the
    // modifier was down, so the moment the modifier lifted it read as a fresh
    // press. Requiring the KEY to have been up last tick kills that whole
    // class of leak — a rising edge now means "this key just went down", and
    // the modifier mask only decides which Action owns that press.
    bool keyNow;
    bool keyLast;
    bool claimed;
};
EdgeState g_edge[static_cast<int>(Action::COUNT)] = {};

bool IsDownVk(int vk) {
    if (vk == 0) return false;
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

// Compute the current modifier mask from OS state. Reads VK_SHIFT (either
// shift), VK_CONTROL (either ctrl), VK_MENU (either alt), and VK_RMENU
// (right Alt = AltGr on QWERTZ) independently so `kModAltGr` can be set
// alongside `kModAlt` when AltGr is held (engine treats them as one Alt;
// we keep them split because announce_facing needs to distinguish).
uint32_t ReadModifiers() {
    uint32_t m = 0;
    if (IsDownVk(VK_SHIFT) || IsDownVk(VK_LSHIFT) || IsDownVk(VK_RSHIFT)) {
        m |= kModShift;
    }
    if (IsDownVk(VK_CONTROL) || IsDownVk(VK_LCONTROL) || IsDownVk(VK_RCONTROL)) {
        m |= kModCtrl;
    }
    if (IsDownVk(VK_MENU) || IsDownVk(VK_LMENU) || IsDownVk(VK_RMENU)) {
        m |= kModAlt;
    }
    if (IsDownVk(VK_RMENU)) {
        m |= kModAltGr;
    }
    return m;
}

// Match a binding against the current keyboard state. VK pressed (primary
// or alt) AND every required mod held AND no forbidden mod held.
// Modifier-blind half of BindingMatches — is the binding's physical key down?
bool BindingKeyDown(const Binding& b) {
    if (b.vk == 0 && b.altVk == 0) return false;
    return IsDownVk(b.vk) || (b.altVk != 0 && IsDownVk(b.altVk));
}

bool BindingMatches(const Binding& b, uint32_t mods) {
    if (b.vk == 0 && b.altVk == 0) return false;
    bool keyDown = IsDownVk(b.vk) || (b.altVk != 0 && IsDownVk(b.altVk));
    if (!keyDown) return false;
    if ((mods & b.modsRequired)  != b.modsRequired)  return false;
    if ((mods & b.modsForbidden) != 0)               return false;
    return true;
}

}  // namespace

// ----- Lifecycle ------------------------------------------------------------

void BeginTick() {
    InitDefaults();
    EnsureOverridesLoaded();
    uint32_t mods = ReadModifiers();
    for (int i = 0; i < static_cast<int>(Action::COUNT); ++i) {
        g_edge[i].now    = BindingMatches(g_bindings[i], mods);
        g_edge[i].keyNow = BindingKeyDown(g_bindings[i]);
    }
}

void EndTick() {
    for (int i = 0; i < static_cast<int>(Action::COUNT); ++i) {
        g_edge[i].last    = g_edge[i].now;
        g_edge[i].keyLast = g_edge[i].keyNow;
        g_edge[i].claimed = false;
    }
}

// ----- Queries --------------------------------------------------------------

bool IsForegroundGame() {
    HWND fg = GetForegroundWindow();
    if (!fg) return false;
    DWORD pid = 0;
    GetWindowThreadProcessId(fg, &pid);
    return pid == GetCurrentProcessId();
}

bool Pressed(Action a) {
    InitDefaults();
    int idx = static_cast<int>(a);
    if (idx < 0 || idx >= static_cast<int>(Action::COUNT)) return false;
    if (g_edge[idx].claimed) return false;
    if (!g_edge[idx].now || g_edge[idx].last) return false;
    // The key itself must have been up last tick — see EdgeState::keyLast.
    if (g_edge[idx].keyLast) return false;
    return IsForegroundGame();
}

bool Held(Action a) {
    InitDefaults();
    int idx = static_cast<int>(a);
    if (idx < 0 || idx >= static_cast<int>(Action::COUNT)) return false;
    return g_edge[idx].now;
}

void Consume(Action a) {
    InitDefaults();
    int idx = static_cast<int>(a);
    if (idx < 0 || idx >= static_cast<int>(Action::COUNT)) return;
    // Force `last == now` so Pressed() returns false for the rest of this
    // tick. EndTick will re-run `last = now`, which is idempotent here.
    g_edge[idx].last = g_edge[idx].now;
}

void ClaimRisingEdge(Action a) {
    InitDefaults();
    int idx = static_cast<int>(a);
    if (idx < 0 || idx >= static_cast<int>(Action::COUNT)) return;
    g_edge[idx].claimed = true;
}

bool ShiftHeld() {
    return IsDownVk(VK_SHIFT) || IsDownVk(VK_LSHIFT) || IsDownVk(VK_RSHIFT);
}

bool CtrlHeld() {
    return IsDownVk(VK_CONTROL) || IsDownVk(VK_LCONTROL) || IsDownVk(VK_RCONTROL);
}

uint32_t CurrentModifiers() {
    return ReadModifiers();
}

bool ModifiedComboOwns(int vk) {
    if (vk == 0) return false;
    InitDefaults();
    uint32_t mods = ReadModifiers();
    // No modifier held → nothing to route away from the engine. (Bare keys are
    // the engine's to handle; we only intercept genuine modifier combos.)
    if ((mods & (kModShift | kModCtrl | kModAlt | kModAltGr)) == 0) return false;
    for (int i = 0; i < static_cast<int>(Action::COUNT); ++i) {
        const Binding& b = g_bindings[i];
        if (b.modsRequired == 0) continue;                 // bare binding — skip
        if (b.vk != vk && b.altVk != vk) continue;         // different key
        // NOTE: we deliberately do NOT require IsDownVk(vk) here. This is called
        // from the engine input hooks, where the engine event ITSELF is proof
        // the key was just pressed — re-checking GetAsyncKeyState races a quick
        // tap (a buffered engine command can reach us after the key already
        // lifted, e.g. Shift+L: L releases before the journal-open command
        // arrives). Shift/Ctrl/Alt are typically still held, so matching on the
        // modifier requirement alone is both sufficient and reliable.
        if ((mods & b.modsRequired)  == b.modsRequired &&
            (mods & b.modsForbidden) == 0) {
            return true;
        }
    }
    return false;
}

// ----- Rebinding ------------------------------------------------------------

Binding Get(Action a) {
    InitDefaults();
    int idx = static_cast<int>(a);
    if (idx < 0 || idx >= static_cast<int>(Action::COUNT)) return {};
    return g_bindings[idx];
}

void Set(Action a, Binding b) {
    InitDefaults();
    int idx = static_cast<int>(a);
    if (idx < 0 || idx >= static_cast<int>(Action::COUNT)) return;
    g_bindings[idx] = b;
    // Reset edge state — a brand-new binding shouldn't fire on the keys
    // the user happens to be holding at the moment of the rebind.
    g_edge[idx].now     = false;
    g_edge[idx].last    = false;
    g_edge[idx].claimed = false;
}

}  // namespace acc::hotkeys

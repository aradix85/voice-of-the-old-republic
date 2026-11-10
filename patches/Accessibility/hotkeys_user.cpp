// User rebinding: the overrides in acc_settings.ini, conflicts and which actions may be rebound.

#include "hotkeys_internal.h"

#include "log.h"                 // acclog::PatchDir / Write — override load gate
#include "mod_settings_store.h"  // persist user rebinds across launches

namespace acc::hotkeys {

namespace {

// ----- User-override persistence --------------------------------------------
// Each rebindable action persists under "Bind_<Name>" in acc_settings.ini as
// "vk,altVk,req,forbid". The English Name() identifiers are stable keys.
bool g_overridesLoaded = false;

void BindKey(Action a, char* out, size_t cap) {
    snprintf(out, cap, "Bind_%s", Name(a));
}

void SerializeBinding(const Binding& b, char* out, size_t cap) {
    snprintf(out, cap, "%d,%d,%u,%u", b.vk, b.altVk,
             static_cast<unsigned>(b.modsRequired),
             static_cast<unsigned>(b.modsForbidden));
}

// Parse "vk,altVk,req,forbid"; returns true on a full 4-field parse.
bool ParseBinding(const char* s, Binding& out) {
    if (!s || !*s) return false;
    int vk = 0, altVk = 0;
    unsigned req = 0, forbid = 0;
    if (sscanf_s(s, "%d,%d,%u,%u", &vk, &altVk, &req, &forbid) != 4) {
        return false;
    }
    out.vk            = vk;
    out.altVk         = altVk;
    out.modsRequired  = req;
    out.modsForbidden = forbid;
    return true;
}

}  // namespace

// Layer persisted user rebinds over the defaults. No-op (and leaves the retry
// flag clear) until the settings path is resolvable, so a launch where this
// runs before the patch dir is known retries on the next tick instead of
// locking in the defaults.
void EnsureOverridesLoaded() {
    if (g_overridesLoaded) return;
    const char* dir = acclog::PatchDir();
    if (!dir || !*dir) return;  // settings file not reachable yet — retry later
    g_overridesLoaded = true;
    int applied = 0;
    for (int i = 0; i < static_cast<int>(Action::COUNT); ++i) {
        Action a = static_cast<Action>(i);
        if (!IsUserRebindable(a)) continue;
        char key[64];
        BindKey(a, key, sizeof(key));
        char val[64];
        if (!acc::settings::GetStr(key, val, sizeof(val))) continue;
        Binding b{};
        if (!ParseBinding(val, b)) continue;
        g_bindings[i] = b;
        ++applied;
    }
    if (applied > 0) {
        acclog::Write("Hotkeys", "loaded %d user rebind(s) from settings",
                      applied);
    }
}

void SetUserBinding(Action a, Binding b) {
    int idx = static_cast<int>(a);
    if (idx < 0 || idx >= static_cast<int>(Action::COUNT)) return;
    Set(a, b);
    char key[64], val[64];
    BindKey(a, key, sizeof(key));
    SerializeBinding(b, val, sizeof(val));
    acc::settings::SetStr(key, val);
    acclog::Write("Hotkeys", "user rebind %s -> %s", Name(a), val);
}

void ResetUserBindings() {
    InitDefaults();
    for (int i = 0; i < static_cast<int>(Action::COUNT); ++i) {
        Action a = static_cast<Action>(i);
        if (!IsUserRebindable(a)) continue;
        Set(a, g_defaults[i]);
        // Overwrite the persisted value with the default so the file reflects
        // the reset state (rather than leaving a stale override behind).
        char key[64], val[64];
        BindKey(a, key, sizeof(key));
        SerializeBinding(g_defaults[i], val, sizeof(val));
        acc::settings::SetStr(key, val);
    }
    acclog::Write("Hotkeys", "all user rebinds reset to defaults");
}

Action FindConflict(Action self, int vk, uint32_t mods) {
    InitDefaults();
    if (vk == 0) return Action::COUNT;
    for (int i = 0; i < static_cast<int>(Action::COUNT); ++i) {
        Action a = static_cast<Action>(i);
        if (a == self) continue;
        if (!IsUserRebindable(a)) continue;
        const Binding& b = g_bindings[i];
        if (b.vk != vk && b.altVk != vk) continue;
        // Would binding b ALSO fire when this exact combo is pressed? That is
        // the double-fire condition: every modifier b requires is held, and no
        // modifier b forbids is held.
        if ((mods & b.modsRequired)  == b.modsRequired &&
            (mods & b.modsForbidden) == 0) {
            return a;
        }
    }
    return Action::COUNT;
}

bool IsUserRebindable(Action a) {
    switch (a) {
    case Action::ProbeAudioCycle:
    case Action::ProbeAudioFire:
    case Action::ProbeCameraDump:
    case Action::ProbeCameraDistDump:
    case Action::ProbeCameraDistClampToggle:
    // Shift+B camera-state probe — a developer diagnostic, not a player
    // hotkey; keep it out of the configurator alongside the F-key probes.
    case Action::CameraStateProbe:
        return false;
    default:
        return true;
    }
}

}  // namespace acc::hotkeys

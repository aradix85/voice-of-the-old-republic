// User rebinding: the overrides in acc_settings.ini, conflicts, and which
// actions may be rebound.

#include "hotkeys_internal.h"

#include "log.h"
#include "mod_settings_store.h"

#include <cstdio>

namespace acc::hotkeys {

namespace {

// Each rebindable action persists as "Bind_<Name>" = "vk,altVk,req,forbid";
// the English Name() is the stable key.
bool g_overridesLoaded = false;

void BindKey(Action a, char* out, size_t cap) {
    snprintf(out, cap, "Bind_%s", Name(a));
}

bool ParseBinding(const char* s, Binding& out) {
    if (!s || !*s) return false;
    int vk = 0, altVk = 0;
    unsigned req = 0, forbid = 0;
    if (sscanf_s(s, "%d,%d,%u,%u", &vk, &altVk, &req, &forbid) != 4) return false;
    out.vk            = vk;
    out.altVk         = altVk;
    out.modsRequired  = req;
    out.modsForbidden = forbid;
    return true;
}

void Persist(Action a, const Binding& b) {
    char key[64], val[64];
    BindKey(a, key, sizeof(key));
    snprintf(val, sizeof(val), "%d,%d,%u,%u", b.vk, b.altVk,
             static_cast<unsigned>(b.modsRequired), static_cast<unsigned>(b.modsForbidden));
    acc::settings::SetStr(key, val);
}

}  // namespace

// Retries every tick until the settings path is known, so an early call does
// not lock in the defaults.
void EnsureOverridesLoaded() {
    if (g_overridesLoaded) return;
    const char* dir = acclog::PatchDir();
    if (!dir || !*dir) return;
    g_overridesLoaded = true;
    int applied = 0;
    for (int i = 0; i < static_cast<int>(Action::COUNT); ++i) {
        const Action a = static_cast<Action>(i);
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
        acclog::Write("Hotkeys", "loaded %d user rebind(s) from settings", applied);
    }
}

void SetUserBinding(Action a, Binding b) {
    const int idx = static_cast<int>(a);
    if (idx < 0 || idx >= static_cast<int>(Action::COUNT)) return;
    Set(a, b);
    Persist(a, b);
    acclog::Write("Hotkeys", "user rebind %s -> %s", Name(a), Describe(a));
}

// The defaults are written back too, so no stale override stays in the file.
void ResetUserBindings() {
    InitDefaults();
    for (int i = 0; i < static_cast<int>(Action::COUNT); ++i) {
        const Action a = static_cast<Action>(i);
        Set(a, g_defaults[i]);
        Persist(a, g_defaults[i]);
    }
    acclog::Write("Hotkeys", "all user rebinds reset to defaults");
}

Action FindConflict(Action self, int vk, uint32_t mods) {
    InitDefaults();
    if (vk == 0) return Action::COUNT;
    for (int i = 0; i < static_cast<int>(Action::COUNT); ++i) {
        const Action a = static_cast<Action>(i);
        if (a == self) continue;
        // The other binding would fire on this combo as well.
        if (UsesKey(g_bindings[i], vk) && ModsSatisfy(g_bindings[i], mods)) return a;
    }
    return Action::COUNT;
}

}  // namespace acc::hotkeys

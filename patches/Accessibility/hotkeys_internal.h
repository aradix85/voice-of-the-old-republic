// Internal to the hotkeys module (hotkeys*.cpp); no other file includes
// it. The public interface is hotkeys.h. Here: what the files share.

#pragma once

#include "hotkeys.h"

#include <cstdint>

namespace acc::hotkeys {

constexpr uint32_t kAnyMod = kModShift | kModCtrl | kModAlt | kModAltGr;

// Indexed by Action. g_bindings is live: the defaults with the user's
// overrides on top. g_defaults keeps the factory values for a reset.
extern Binding g_bindings[static_cast<int>(Action::COUNT)];
extern Binding g_defaults[static_cast<int>(Action::COUNT)];

// True when `b` sits on key `vk`, as its primary or its alternate key.
inline bool UsesKey(const Binding& b, int vk) {
    return b.vk == vk || b.altVk == vk;
}

// True when `mods` holds every modifier `b` requires and none it forbids.
inline bool ModsSatisfy(const Binding& b, uint32_t mods) {
    return (mods & b.modsRequired) == b.modsRequired && (mods & b.modsForbidden) == 0;
}

// Helpers shared across the files, by the file that defines them.

// hotkeys_defaults.cpp
void InitDefaults();  // once; every entry point calls it first

// hotkeys_user.cpp
void EnsureOverridesLoaded();

}  // namespace acc::hotkeys

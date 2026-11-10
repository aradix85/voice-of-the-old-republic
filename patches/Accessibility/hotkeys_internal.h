// Internal to the hotkeys module (hotkeys*.cpp); no other file includes
// it. The public interface is hotkeys.h. Here: what the files share.

#pragma once

#include "hotkeys.h"

#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>


#pragma comment(lib, "user32.lib")

namespace acc::hotkeys {

// ----- Default bindings ------------------------------------------------------
// One row per `Action`. Indexed by static_cast<int>(Action). Edit defaults
// here — every call site reads through `g_bindings`, so changing a row
// here changes the binding mod-wide. Future rebind UI overwrites at runtime
// via `Set()`.
extern Binding g_bindings[static_cast<int>(Action::COUNT)];

// Factory defaults, frozen at InitDefaults time before any user override is
// applied. The configurator's "restore defaults" copies these back into
// g_bindings. Kept separate from g_bindings so a runtime rebind never loses the
// original value.
extern Binding g_defaults[static_cast<int>(Action::COUNT)];

// Helpers shared across the files, by the file that defines them.

// hotkeys_defaults.cpp
void InitDefaults();

// hotkeys_user.cpp
void EnsureOverridesLoaded();

}  // namespace acc::hotkeys

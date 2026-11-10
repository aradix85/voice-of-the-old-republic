// Internal to the core_tick module (core_tick*.cpp); no other file includes
// it. The public interface is core_tick.h. Here: what the files share.

#pragma once

#include "core_tick.h"

#include <windows.h>

#include <cmath>   // world-response liveness (dead-keyboard watchdog)


// core_dllmain.cpp — the once-per-session stand-up (guards, speech, language,
// config snapshot, keymap, focus probe, bring-up nag). Forward-declared the
// same way EnsureSpeechInitialized is in the menu translation units.
void EnsureSessionBringup();

namespace acc::tick {

// Helpers shared across the files, by the file that defines them.

// core_tick_keyboard.cpp
void RetryColdStartReacquire();
void WatchDeadKeyboard();

}  // namespace acc::tick

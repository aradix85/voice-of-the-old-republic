// Internal to the core_tick module (core_tick*.cpp); no other file includes
// it. The public interface is core_tick.h.

#pragma once

#include "core_tick.h"

namespace acc::tick {

// core_tick_keyboard.cpp, called once per tick before any input poll.
void RetryColdStartReacquire();
void WatchDeadKeyboard();

}  // namespace acc::tick

// Internal to the cycle_input module (cycle_input*.cpp); no other file includes
// it. The public interface is cycle_input.h. Here: what the files share.

#pragma once

#include "cycle_input.h"

#include <windows.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "engine_compass.h"
#include "filter_objects.h"

namespace acc::cycle_input {

// The o'clock of a target, 1..12 with 12 directly ahead (engine_compass).
// The yaw fed in is the CAMERA's, via view_mode::GetEffectiveViewpoint —
// never the creature's: KOTOR's A/D orbits the camera and leaves the body
// pointing where it was until W/S commits a step, so a body-anchored clock
// would not move when the user turns.
using acc::engine::ClockPosition;

// Map a CycleCategory to its strings::Id pair (singular name + empty
// message). Centralises the table so OnCycleItem / OnCycleCategory don't
// each carry duplicate switches.
struct CategoryBindings {
    acc::strings::Id  name;    // singular for prefix / fallback
    acc::strings::Id  empty;   // full pre-formatted "no X in range"
};

// Helpers shared across the files, by the file that defines them.

// cycle_input.cpp
CategoryBindings BindingsFor(acc::filter::CycleCategory c,
                             acc::filter::CycleContext ctx = acc::filter::CycleContext::World);

// cycle_input_announce.cpp
int FormatItemPayload(const char* name, bool haveYaw, int clock, int metres,
                      char* outBuf, size_t outBufSize);
void OnCycleItem(bool prev, acc::filter::CycleContext ctx);
void OnCycleEdge(bool last, acc::filter::CycleContext ctx);
void OnCycleCategory(bool prev, acc::filter::CycleContext ctx);

// cycle_input_focus.cpp
void OnAnnounceFocus();
void OnPathfindFocus();
void OnRouteFocus();
void OnPathfindFocusForce();

}  // namespace acc::cycle_input

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

// user32.lib for GetAsyncKeyState / GetForegroundWindow /
// GetWindowThreadProcessId. The patch build script (create-patch.bat) links
// only sqlite3 by default; pragma keeps the link config local to the TU
// that needs it.
#pragma comment(lib, "user32.lib")

#include "engine_compass.h"      // ClockPosition — shared clock-direction math
#include "filter_objects.h"

namespace acc::cycle_input {

// Clock-position relative to the user's facing, returning 1..12 with 12 =
// directly ahead, 3 = to the right, 6 = directly behind, 9 = to the left.
//
// Yaw frame: degrees CCW from +X (engine convention). World-frame angle of
// the (object - viewpoint) vector is atan2(dy, dx). Their difference is the
// relative bearing CCW from "ahead"; clocks tick CW, so we negate before
// bucketing into 30° slots.
//
// The yaw fed in is the CAMERA's, via view_mode::GetEffectiveViewpoint —
// never the creature's. KOTOR's A/D orbits the camera and leaves the body
// pointing where it was until W/S commits a step, so a body-anchored clock
// does not move when the user turns: every object keeps the o'clock it had
// before the turn until the next step snaps the body. That was the reported
// "clock announcements go stale" bug.
//
// Returns 12 for the directly-ahead bucket (which would otherwise be 0).
// Shared definition lives in engine_compass (also used by trap_watch).
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
CategoryBindings BindingsFor( acc::filter::CycleCategory c, acc::filter::CycleContext ctx = acc::filter::CycleContext::World);

// cycle_input_announce.cpp
int FormatItemPayload(const char* name, bool haveYaw, int clock, int metres, char* outBuf, size_t outBufSize);
void OnCycleItem(bool prev, acc::filter::CycleContext ctx);
void OnCycleEdge(bool last, acc::filter::CycleContext ctx);
void OnCycleCategory(bool prev, acc::filter::CycleContext ctx);

// cycle_input_focus.cpp
void OnAnnounceFocus();
void OnPathfindFocus();
void OnRouteFocus();
void OnPathfindFocusForce();

}  // namespace acc::cycle_input

// Internal to the view_mode module (view_mode*.cpp); no other file includes
// it. The public interface is view_mode.h. Here: what the files share.

#pragma once

#include "view_mode.h"

#include <windows.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#pragma comment(lib, "user32.lib")


namespace acc::view_mode {

constexpr float kCursorSpeedMps = 2.0f;       // matches KOTOR walk pace
constexpr float kMaxDtSec = 0.1f;             // guard against teleport-on-stall
constexpr float kHoverRadiusMeters = 1.0f;    // cursor must be on the object

struct ViewModeState {
    bool   active            = false;
    Vector cursor_pos        = { 0.0f, 0.0f, 0.0f };
    float  cursor_yaw_deg    = 0.0f;   // engine frame: 0° = +X, CCW positive
    DWORD  last_tick_ms      = 0;
    bool   at_wall           = false;  // "Wall" said for this contact
    // Hover-pause object debounce. Both handle and CSWSObject* are written
    // together so Enter dispatch reads them as a pair without re-resolving.
    uint32_t hover_last_spoken     = 0;
    uint32_t hover_pending         = 0;
    void*    hover_pending_obj     = nullptr;
    DWORD    hover_pending_started = 0;
    // Region-cursor announce. Text-equality dedup so adjacent rooms with
    // identical labels collapse to one announce.
    std::string region_pending_text;
    DWORD region_pending_started_ms  = 0;
    std::string region_last_spoken_text;
};

extern ViewModeState g_state;

// Helpers shared across the files, by the file that defines them.

// view_mode_narrate.cpp
void AnnounceCursorRegion(void* area, const Vector& cursor);
void NarrateNearestObject(void* area, const Vector& cursor);

// view_mode_enter.cpp
void PollEnter();
void ProcessPendingDispatch();

}  // namespace acc::view_mode

// Internal to the view_mode module (view_mode*.cpp); no other file includes
// it. The public interface is view_mode.h. Here: what the files share.

#pragma once

#include "view_mode.h"

#include <windows.h>

#include <cstdint>
#include <string>

namespace acc::view_mode {

constexpr float kHoverRadiusMeters = 1.0f;    // the cursor must be on the object

struct ViewModeState {
    bool   active            = false;
    Vector cursor_pos        = { 0.0f, 0.0f, 0.0f };
    float  cursor_yaw_deg    = 0.0f;   // engine frame: 0° = +X, CCW positive
    DWORD  last_tick_ms      = 0;
    bool   at_wall           = false;  // "Wall" said for this contact
    // Hover-pause object debounce. Handle and CSWSObject* are written
    // together so Enter dispatch reads them as a pair without re-resolving.
    uint32_t hover_last_spoken     = 0;
    uint32_t hover_pending         = 0;
    void*    hover_pending_obj     = nullptr;
    DWORD    hover_pending_started = 0;
    // Region announce, deduplicated on the text, so adjacent rooms with the
    // same label give one announcement.
    std::string region_pending_text;
    DWORD region_pending_started_ms  = 0;
    std::string region_last_spoken_text;
};

// view_mode.cpp
extern ViewModeState g_state;

// Helpers shared across the files, by the file that defines them.

// view_mode_narrate.cpp
void AnnounceCursorRegion(void* area, const Vector& cursor);
void NarrateNearestObject(void* area, const Vector& cursor);

// view_mode_enter.cpp
void PollEnter();
void ProcessPendingDispatch();  // runs before the active gate: view mode has exited

}  // namespace acc::view_mode

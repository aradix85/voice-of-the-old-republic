// Map UI cursor: the session, the move keys, PanToWorld and the per-tick
// flow (move the cursor, find what is under it, speak it after the hover
// pause).
//
// The cursor lives in map-pixel space [0..440] x [0..256]. The move keys
// translate it with map-frame axes (forward = up on the map): a 2D scan UI,
// not a 3D heading.

#include "map_ui_cursor_internal.h"

#include <cmath>
#include <cstdio>

#include "engine_area.h"
#include "engine_keymap.h"
#include "engine_panels.h"
#include "engine_player.h"
#include "engine_reads.h"
#include "map_shipped_hints.h"
#include "speech.h"
#include "strings.h"

namespace acc::map_ui_cursor {

CursorState g_state;

namespace {

// Pixels per second. The map render fills ~440x256 of KOTOR's 640x480
// reference resolution; 100 px/s crosses it in ~4.4 s, comparable to the
// engine walk speed.
constexpr float kCursorSpeedPx = 100.0f;

// dt cap against a jump after a stall (loading screen, alt-tab).
constexpr float kMaxDtSec = 0.1f;

// Keys are only consumed while KOTOR has focus, so alt-tabbing does not
// steal movement from other apps.
bool IsForegroundProcess() {
    HWND fg = GetForegroundWindow();
    if (!fg) return false;
    DWORD fgPid = 0;
    GetWindowThreadProcessId(fg, &fgPid);
    return fgPid == GetCurrentProcessId();
}

// True if the PRIMARY key bound to the movement axis is held: the map cursor
// follows the keys the player walks with (forward → up, back → down,
// strafe-left → left, strafe-right → right), WASD by default. Not the full
// MoveAxisVks union: that also holds KOTOR's arrow-key alternates, and on
// this screen the arrows step the menu chain (menus_dispatch hands Up/Down
// to HandleNavStep with no InGameMap gate) — with them, one Down press
// panned the cursor south AND stepped the chain.
bool AxisDown(acc::engine_keymap::MoveAxis axis) {
    const int vk = acc::engine_keymap::MoveAxisPrimaryVk(axis);
    return vk != 0 && (GetAsyncKeyState(vk) & 0x8000) != 0;
}

// Clamps a map pixel into the map; true if it had to.
bool ClampToMap(float& px, float& py) {
    bool clamped = false;
    if (px < 0.0f)                 { px = 0.0f;                 clamped = true; }
    if (px > (float)kMapPixelMaxX) { px = (float)kMapPixelMaxX; clamped = true; }
    if (py < 0.0f)                 { py = 0.0f;                 clamped = true; }
    if (py > (float)kMapPixelMaxY) { py = (float)kMapPixelMaxY; clamped = true; }
    return clamped;
}

void SeedCursorAtPlayer(void* areaMap) {
    Vector p;
    if (!acc::engine::GetPlayerPosition(p)) return;
    float px, py;
    if (!WorldToPixel(areaMap, p, px, py)) return;
    ClampToMap(px, py);
    g_state.px    = px;
    g_state.py    = py;
    g_state.world = p;
}

// A fresh session state; the cursor keeps its position.
void ResetSessionState() {
    const float  px    = g_state.px;
    const float  py    = g_state.py;
    const Vector world = g_state.world;
    g_state = CursorState{};
    g_state.px    = px;
    g_state.py    = py;
    g_state.world = world;
}

void Deactivate(const char* why) {
    if (!g_state.active) return;
    acclog::Write("MapCursor", "deactivated — %s", why);
    ResetSessionState();
}

void ClearPendingNote() {
    g_state.pending_note            = nullptr;
    g_state.pending_note_is_pin     = false;
    g_state.pending_note_started_ms = 0;
}

// Session start: seed the cursor at the player, speak the area name, and
// make sure the region shapes are built.
void StartSession(void* areaMap, DWORD now) {
    SeedCursorAtPlayer(areaMap);
    PixelToWorld(areaMap, g_state.px, g_state.py, g_state.world.z, g_state.world);
    ResetSessionState();
    g_state.active       = true;
    g_state.last_tick_ms = now;
    acclog::Write("MapCursor",
                  "activated — seed pixel=(%.1f,%.1f) world=(%.2f,%.2f,%.2f)",
                  g_state.px, g_state.py,
                  g_state.world.x, g_state.world.y, g_state.world.z);

    void* area = acc::engine::GetCurrentArea();
    if (!area) return;
    // The area title, what a sighted player reads off the green banner
    // ("Taris - Southern Apartments"). The engine's first-sight announce
    // says "Map" the instant the map opens; interrupt=false queues the name
    // after it.
    char name[160] = {0};
    if (acc::engine::GetAreaDisplayName(area, name, sizeof(name)) && name[0] != '\0') {
        acc::speech::Speak(name, /*interrupt=*/false);
        acclog::Write("MapCursor", "area name=\"%s\"", name);
    }
    // transitions::Tick normally built the region shapes on area entry; this
    // covers a map opened before it ran. Idempotent per area.
    acc::room_topology::BuildForArea(area);
}

// Moves the cursor by the held move keys; true if it moved. Pushing into
// the map boundary says so once per push, not on an idle clamp.
bool MoveCursor(void* areaMap, DWORD now) {
    float dt = (now - g_state.last_tick_ms) * 0.001f;
    g_state.last_tick_ms = now;
    if (dt > kMaxDtSec) dt = kMaxDtSec;

    // Map-frame axes: forward is up on the map (pixel y grows down).
    using acc::engine_keymap::MoveAxis;
    const bool w = AxisDown(MoveAxis::Forward);
    const bool s = AxisDown(MoveAxis::Backward);
    const bool a = AxisDown(MoveAxis::TurnLeft);
    const bool d = AxisDown(MoveAxis::TurnRight);
    if (w || s || a || d) {
        acclog::Trace("MapCursor", "input W=%d S=%d A=%d D=%d", w, s, a, d);
    }
    float vx = 0.0f, vy = 0.0f;
    if (w && !s) vy -= 1.0f;
    if (s && !w) vy += 1.0f;
    if (a && !d) vx -= 1.0f;
    if (d && !a) vx += 1.0f;
    if (vx != 0.0f || vy != 0.0f) {
        // A diagonal is not 1.4 times as fast.
        const float mag = std::sqrt(vx * vx + vy * vy);
        vx /= mag;
        vy /= mag;
    }
    const float step = kCursorSpeedPx * dt;
    float px = g_state.px + vx * step;
    float py = g_state.py + vy * step;
    const bool clamped = ClampToMap(px, py);
    const bool moved   = px != g_state.px || py != g_state.py;
    g_state.px = px;
    g_state.py = py;
    PixelToWorld(areaMap, px, py, g_state.world.z, g_state.world);

    const bool pushingEdge = clamped && (vx != 0.0f || vy != 0.0f);
    if (pushingEdge && !g_state.at_edge) {
        acc::speech::Speak(acc::strings::Get(acc::strings::Id::MapCursorEdge),
                           /*interrupt=*/true);
        acclog::Write("MapCursor", "edge pixel=(%.1f,%.1f)", px, py);
    }
    g_state.at_edge = pushingEdge;
    return moved;
}

// The note under the cursor: the nearest map note, user marker or shipped
// hint, the closest of the three. outIsPin picks the pin's text accessor at
// speak time; a shipped hint is recognised there by IsShippedHint.
void* FindHoverHit(void* mapPanel, void* areaMap, bool moved, bool& outIsPin) {
    Nearest note;
    {
        // Folds the per-tick waypoint dump to one "(repeated Nx —
        // unchanged)" line while the map sits still.
        acclog::BlockLog dump("MapCursor.dump");
        note = FindNearestExploredMapNote(mapPanel, areaMap, g_state.px, g_state.py, dump);
    }
    void* serverArea = acc::engine::GetCurrentArea();
    void* clientArea = serverArea ? acc::engine::GetClientArea(serverArea) : nullptr;
    const Nearest pin  = FindNearestUserMapPin(clientArea, areaMap, g_state.px, g_state.py);
    const Nearest hint = FindNearestShippedHint(areaMap, g_state.px, g_state.py);

    Nearest best = note;
    const char* kind = "waypoint";
    outIsPin = false;
    if (pin.obj && (!best.obj || pin.dist2 < best.dist2)) {
        best = pin;
        kind = "pin";
        outIsPin = true;
    }
    if (hint.obj && (!best.obj || hint.dist2 < best.dist2)) {
        best = hint;
        kind = "hint";
        outIsPin = false;
    }
    if (moved) {
        acclog::Trace("MapCursor",
                      "tick pixel=(%.1f,%.1f) world=(%.2f,%.2f) "
                      "scanned=%d pins=%d hints=%d hit=%p kind=%s",
                      g_state.px, g_state.py, g_state.world.x, g_state.world.y,
                      note.scanned, pin.scanned, hint.scanned, best.obj, kind);
    }
    return best.obj;
}

// The region shape at the cursor, from the source the in-world walking
// adapter speaks from, so cursor and walking agree on corridor, junction
// or dead end.
Shape ProbeShape(void* areaMap) {
    Shape shape;
    if (!acc::engine::IsWorldPointExplored(areaMap, g_state.world)) return shape;
    void* area = acc::engine::GetCurrentArea();
    if (!area) return shape;
    int sig = 0;
    shape.have = acc::room_topology::LookupAt(area, g_state.world, shape.text,
                                              sig, shape.clusterId,
                                              /*allowDiagLog=*/false,
                                              /*requireWallReachable=*/false);
    return shape;
}

// Speaks the note under the cursor: its text, or a generic name when it has
// none (an enabled note the engine draws must not stay silent), then the
// region shape.
void SpeakNote(void* hit, bool isPin, const Shape& shape) {
    const bool isHint = acc::map_shipped_hints::IsShippedHint(hit);
    char text[256] = {0};
    char tag[128]  = {0};
    bool haveText;
    acc::strings::Id fallback = acc::strings::Id::MapCursorWaypointPOI;
    if (isHint) {
        haveText = acc::map_shipped_hints::GetLabel(hit, text, sizeof(text));
    } else if (isPin) {
        haveText = acc::engine::GetMapPinNoteText(hit, text, sizeof(text));
        fallback = acc::strings::Id::MapPinNoText;
    } else {
        haveText = acc::engine::GetWaypointMapNote(hit, text, sizeof(text));
        if (!acc::engine::ReadCExoString(hit, kObjectTagOffset, tag, sizeof(tag))) {
            tag[0] = '\0';   // the tag is for the log only
        }
    }
    haveText = haveText && text[0] != '\0';
    if (!haveText) {
        std::snprintf(text, sizeof(text), "%s", acc::strings::Get(fallback));
    }
    acc::speech::Speak(WithShape(text, shape).c_str(), /*interrupt=*/true);
    acclog::Write("MapCursor",
                  "speak %s=\"%s\" tag=\"%s\" shape=\"%s\" haveText=%d "
                  "cursor=(%.1f,%.1f)",
                  isHint ? "hint" : (isPin ? "pin" : "note"), text, tag,
                  shape.have ? shape.text.c_str() : "", (int)haveText,
                  g_state.px, g_state.py);
}

// The hover pause on a note: arm on a new note, speak once it has held for
// kHoverPauseMs, then stay silent on it until the cursor leaves.
void TickNoteHover(void* hit, bool hitIsPin, const Shape& shape, DWORD now) {
    // The note is the more specific information: an armed ambient starts
    // afresh once the cursor leaves it.
    ClearPendingAmbient();
    if (hit == g_state.last_spoken_note) {
        ClearPendingNote();
        return;
    }
    if (hit != g_state.pending_note) {
        g_state.pending_note            = hit;
        g_state.pending_note_is_pin     = hitIsPin;
        g_state.pending_note_started_ms = now;
        return;
    }
    if (g_state.pending_note_started_ms == 0 ||
        now - g_state.pending_note_started_ms < kHoverPauseMs) {
        return;
    }
    SpeakNote(hit, g_state.pending_note_is_pin, shape);
    g_state.last_spoken_note = hit;
    ClearPendingNote();
    // A note in between re-arms the ambient: back in the zone the cursor came
    // from, it is announced again.
    ClearLastSpokenAmbient();
}

}  // namespace

std::string WithShape(const std::string& text, const Shape& shape) {
    if (!shape.have || shape.text.empty()) return text;
    // Two sentences, so the screen reader pauses between them.
    return text + ". " + shape.text;
}

bool TryGetCursorWorldPosition(Vector& out) {
    if (!g_state.active) return false;
    out = g_state.world;
    return true;
}

void PanToWorld(const Vector& world, void* suppressWaypoint) {
    if (!g_state.active) return;

    void* areaMap = acc::engine::GetAreaMap();
    if (!areaMap) return;

    float px, py;
    if (!WorldToPixel(areaMap, world, px, py)) return;
    // A transition at the world edge of the mapped region can project just
    // outside the map; the cursor settles on the edge.
    ClampToMap(px, py);
    g_state.px    = px;
    g_state.py    = py;
    g_state.world = world;

    // The cycle just spoke: no hover pause may repeat the name (waypoint) or
    // add a shape tail to the cycle's category line (door, transition).
    ClearPendingNote();
    ClearPendingAmbient();
    // A waypoint landing counts as spoken until the cursor leaves it; other
    // landings keep last_spoken_note, so coming off the pan onto a real note
    // still announces. The ambient re-arms, so sitting still after the pan
    // gives a fresh region cue.
    if (suppressWaypoint) g_state.last_spoken_note = suppressWaypoint;
    ClearLastSpokenAmbient();

    acclog::Write("MapCursor",
                  "pan to=(%.2f,%.2f) pixel=(%.1f,%.1f) suppressWaypoint=%p",
                  world.x, world.y, px, py, suppressWaypoint);
}

void Tick() {
    // The map sub-screen sits under the CSWGuiInGameMenu strip, so the
    // foreground panel is the strip: look for the map panel itself.
    void* mapPanel = IsForegroundProcess()
        ? acc::engine::FindPanelByKind(acc::engine::PanelKind::InGameMap)
        : nullptr;
    if (!mapPanel) {
        Deactivate("map no longer foreground");
        return;
    }
    void* areaMap = acc::engine::GetAreaMap();
    if (!areaMap) {
        Deactivate("area_map vanished");
        return;
    }

    const DWORD now = GetTickCount();
    if (!g_state.active) StartSession(areaMap, now);

    const bool moved = MoveCursor(areaMap, now);
    bool hitIsPin = false;
    void* hit = FindHoverHit(mapPanel, areaMap, moved, hitIsPin);
    const Shape shape = ProbeShape(areaMap);
    if (hit) {
        TickNoteHover(hit, hitIsPin, shape, now);
        return;
    }
    // Off every note: drop an armed one, and once the cursor moves, the last
    // spoken note announces again on return.
    ClearPendingNote();
    if (moved) g_state.last_spoken_note = nullptr;
    TickAmbientHover(areaMap, shape, now);
}

}  // namespace acc::map_ui_cursor

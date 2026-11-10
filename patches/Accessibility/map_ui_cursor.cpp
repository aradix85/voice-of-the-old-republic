// Phase 5 lay-off 6 — virtual cursor on the area map UI.
//
// See map_ui_cursor.h for the user model and engine surfaces.
//
// Cursor lives in map-pixel space [0..440] × [0..256]. Movement keys
// translate the cursor in pixel space (with map-frame-relative axes —
// W = "up on the map", S = "down", A = "left", D = "right"; this is a
// 2D scan UI, not a 3D heading). The cursor's world projection comes
// from inverting CSWSAreaMap's pixel transform (4 fields, see
// docs/navsystems-investigation.md §Q4).

#include "map_ui_cursor_internal.h"

#include "engine_area.h"
#include "engine_keymap.h"            // MoveAxisVks — bound move/turn keys
#include "engine_panels.h"
#include "engine_player.h"
#include "view_mode.h"   // kHoverPauseMs — shared cursor cadence
#include "strings.h"
#include "speech.h"
#include "transitions.h"
#include "room_topology.h"            // nav-graph region lookup — same source

namespace acc::map_ui_cursor {

namespace {

// Movement speed in pixels per second. KOTOR's reference resolution is
// 640×480 and the map render fills ~440×256 of that; a 100 px/s cursor
// crosses the map in ~4.4 s, comparable to the engine walk speed. Tune
// live.
constexpr float kCursorSpeedPx = 100.0f;

// Bounds of the engine's map-pixel coordinate system (per Q4). Inclusive
// upper bound matches the engine's own check `< 0x1b9` / `< 0x101`, so
// the highest legal cursor pixel is (440, 256).
constexpr int kMapPixelMaxX = 440;
constexpr int kMapPixelMaxY = 256;

// dt cap to prevent teleport-on-stall (loading screens, alt-tab).
constexpr float kMaxDtSec = 0.1f;

// Hover-pause debounce for cursor narration — shared with view mode so
// spoken cadence stays consistent across our two cursor surfaces. See
// view_mode.h for the value and rationale.
using acc::view_mode::kHoverPauseMs;

struct CursorState {
    bool   active                  = false;
    bool   announced_area_name     = false;
    float  px                      = 220.0f;   // (kMapPixelMaxX / 2.0f)
    float  py                      = 128.0f;   // (kMapPixelMaxY / 2.0f)
    Vector world                   = {0, 0, 0};
    DWORD  last_tick_ms            = 0;
    bool   at_edge                 = false;  // "Edge of the map" said for this push
    // Waypoint hover-pause (explicit map-note overlay — Tier 1
    // landmark-as-game-object).
    // `pending_note_is_pin` / `last_spoken_is_pin` discriminate
    // CSWSWaypoint hits (pin=false) from user CSWCMapPin hits (pin=true).
    // The state machine keys off the pointer identity for dedup; the
    // kind flag only steers the text-read at speak time so the right
    // engine_area accessor (GetWaypointMapNote vs GetMapPinNoteText)
    // gets called.
    void*  pending_note_waypoint   = nullptr;
    bool   pending_note_is_pin     = false;
    DWORD  pending_note_started_ms = 0;
    void*  last_spoken_waypoint    = nullptr;
    bool   last_spoken_is_pin      = false;
    // Ambient hover-pause (fog-of-war + layout-room labels + terrain
    // shape). A single timer covers all kinds so flipping between e.g.
    // Landmark and RoomName doesn't double-debounce. last_spoken_*
    // latches the most recent ambient announcement; the cursor stays
    // silent while it sits inside the same ambient zone, and re-arms
    // the moment a different kind/key is observed.
    //
    // pending_ambient_room_idx is the identity key, repurposed across
    // kinds: flat-cache landmark index for Landmark, .lyt-room index
    // for RoomName, room_topology cluster id for TerrainShape (stable
    // per perceptual region — no sig drift inside a curved corridor),
    // -1 otherwise. The kind tag disambiguates numerically-overlapping
    // values across these three namespaces, so renaming the field
    // isn't needed.
    AmbientKind pending_ambient_kind         = AmbientKind::None;
    int         pending_ambient_room_idx     = -1;
    DWORD       pending_ambient_started_ms   = 0;
    AmbientKind last_spoken_ambient_kind     = AmbientKind::None;
    int         last_spoken_ambient_room_idx = -1;
    // Text-based dedup overlay: when adjacent rooms produce identical
    // labels ("junction, North, East" in three Taris rooms with
    // different signatures), the (kind, sig) comparator alone treats
    // them as distinct and re-announces on every crossing. Storing
    // the last-spoken text and comparing case-sensitively collapses
    // those into one logical region — same text, no re-announce.
    std::string last_spoken_ambient_text;
    // TerrainShape speak buffer: the cluster's description is built at
    // probe time (when we arm the hover-pause), cached on the pending
    // side, and consumed when the timer elapses. room_topology builds
    // the label per cluster at BuildForArea time, so the text is stable
    // across the cluster's footprint — capturing it once at arm is
    // sufficient. Identity / dedup lives in pending_ambient_room_idx
    // (cluster id for TerrainShape, see comment above).
    std::string pending_shape_text;
};

CursorState g_state;

// Region shape is served by room_topology (nav-graph decomposition,
// built once on area-enter by transitions::Tick). The in-world walking
// adapter speaks from the same source — cursor and walking can never
// disagree about whether a position is a corridor / junction / dead-end.

// Foreground-window gate. Same pattern as cycle_input::PollWin32 — only
// consume keys while KOTOR has focus so alt-tabbing doesn't steal
// movement from other apps.
bool IsForegroundProcess() {
    HWND fg = GetForegroundWindow();
    if (!fg) return false;
    DWORD fgPid = 0;
    GetWindowThreadProcessId(fg, &fgPid);
    return fgPid == GetCurrentProcessId();
}

// The InGameMap sub-screen is pushed onto panels[] *under* the
// CSWGuiInGameMenu strip — GetForegroundPanel returns the strip, not
// the map. Walk panels[] explicitly looking for any InGameMap entry;
// that's how `MonitorPanelContents` finds the sub-screen too.
bool IsMapPanelActive(void** outPanel) {
    void* p = acc::engine::FindPanelByKind(acc::engine::PanelKind::InGameMap);
    if (outPanel) *outPanel = p;
    return p != nullptr;
}

void SeedCursorAtPlayer(void* areaMap) {
    Vector p;
    if (!acc::engine::GetPlayerPosition(p)) return;
    float px, py;
    if (!WorldToPixel(areaMap, p, px, py)) return;
    if (px < 0) px = 0;
    if (px > (float)kMapPixelMaxX) px = (float)kMapPixelMaxX;
    if (py < 0) py = 0;
    if (py > (float)kMapPixelMaxY) py = (float)kMapPixelMaxY;
    g_state.px = px;
    g_state.py = py;
    g_state.world = p;
}

void ResetSessionState() {
    g_state.active                        = false;
    g_state.announced_area_name           = false;
    g_state.last_tick_ms                  = 0;
    g_state.at_edge                       = false;
    g_state.pending_note_waypoint         = nullptr;
    g_state.pending_note_started_ms       = 0;
    g_state.last_spoken_waypoint          = nullptr;
    g_state.pending_ambient_kind          = AmbientKind::None;
    g_state.pending_ambient_room_idx      = -1;
    g_state.pending_ambient_started_ms    = 0;
    g_state.last_spoken_ambient_kind      = AmbientKind::None;
    g_state.last_spoken_ambient_room_idx  = -1;
    g_state.last_spoken_ambient_text.clear();
    g_state.pending_shape_text.clear();
}

bool KeyDown(int vk) {
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

// True if the key bound to the given movement direction is held. Drives the
// map cursor with the SAME keys the player uses to walk in the world
// (forward→up, back→down, strafe-left→left, strafe-right→right) so it follows
// a rebind; the WASD defaults are the fallback, so it never regresses.
//
// PRIMARY bind, not the full MoveAxisVks union: that union also holds KOTOR's
// arrow-key movement alternates (Action285/286), and the arrows have their own
// job on this screen. Chain navigation runs on the map panel — menus_dispatch
// hands Up/Down to HandleNavStep with no InGameMap gate — and it reaches the
// chain through the engine's dispatched nav code, while this reads Win32 async
// state. Two independent paths, one keypress: Down used to pan the cursor
// south AND step the chain in the same press. Taking the alternates out here
// leaves the arrows to the menu and W/S/A/D to the cursor.
bool AxisDown(acc::engine_keymap::MoveAxis axis) {
    int vk = acc::engine_keymap::MoveAxisPrimaryVk(axis);
    return vk != 0 && KeyDown(vk);
}

}  // namespace

bool IsActive() { return g_state.active; }

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

    // Clamp inside the map-pixel rectangle so the cursor visibly settles
    // on the object even if the projection falls slightly off-bounds
    // (rare, but happens for transitions sitting at the world-edge of an
    // area's mapped region).
    if (px < 0.0f) px = 0.0f;
    if (px > (float)kMapPixelMaxX) px = (float)kMapPixelMaxX;
    if (py < 0.0f) py = 0.0f;
    if (py > (float)kMapPixelMaxY) py = (float)kMapPixelMaxY;

    g_state.px    = px;
    g_state.py    = py;
    g_state.world = world;

    // Cancel any in-flight hover-pause — the cycle just spoke. We don't
    // want the cursor's own debounce to fire 300 ms later and re-announce
    // the same name (waypoint case) or contradict the cycle's category
    // narration with a terrain-shape tail (door / transition case).
    g_state.pending_note_waypoint      = nullptr;
    g_state.pending_note_started_ms    = 0;
    g_state.pending_ambient_kind       = AmbientKind::None;
    g_state.pending_ambient_room_idx   = -1;
    g_state.pending_ambient_started_ms = 0;
    g_state.pending_shape_text.clear();

    // Waypoint landing — latch as already-spoken so the cursor stays
    // silent on this pin until the user pans away and back. Non-
    // waypoint landings (Door / Transition) leave last_spoken_waypoint
    // untouched so coming off the pan back onto a real map-note still
    // announces. Ambient latching cleared so the user does get a fresh
    // surrounding-room cue if they sit still after the pan.
    if (suppressWaypoint) {
        g_state.last_spoken_waypoint = suppressWaypoint;
    }
    g_state.last_spoken_ambient_kind     = AmbientKind::None;
    g_state.last_spoken_ambient_room_idx = -1;
    g_state.last_spoken_ambient_text.clear();

    acclog::Write("MapCursor",
                  "pan to=(%.2f,%.2f) pixel=(%.1f,%.1f) "
                  "suppressWaypoint=%p",
                  world.x, world.y, px, py, suppressWaypoint);
}

void Tick() {
    void* mapPanel = nullptr;
    if (!IsForegroundProcess() || !IsMapPanelActive(&mapPanel)) {
        if (g_state.active) {
            acclog::Write("MapCursor", "deactivated — map no longer foreground");
            ResetSessionState();
        }
        return;
    }

    void* areaMap = acc::engine::GetAreaMap();
    if (!areaMap) {
        if (g_state.active) {
            acclog::Write("MapCursor", "deactivated — area_map vanished");
            ResetSessionState();
        }
        return;
    }

    DWORD now = GetTickCount();
    if (!g_state.active) {
        SeedCursorAtPlayer(areaMap);
        PixelToWorld(areaMap, g_state.px, g_state.py, g_state.world.z, g_state.world);
        ResetSessionState();
        g_state.active       = true;
        g_state.last_tick_ms = now;
        acclog::Write("MapCursor",
                      "activated — seed pixel=(%.1f,%.1f) world=(%.2f,%.2f,%.2f)",
                      g_state.px, g_state.py,
                      g_state.world.x, g_state.world.y, g_state.world.z);

        // Speak the area title once per map-open. This is what a sighted
        // player gets from the green banner ("Taris - Southern
        // Apartments"); blind players need it announced or they lose the
        // "which map am I looking at" anchor.
        //
        // The engine's own panel first-sight announce speaks the map title
        // ("Map") the instant the map opens; interrupt=false queues the
        // area name AFTER it, so the user hears "Map" then the area name.
        void* area = acc::engine::GetCurrentArea();
        if (!g_state.announced_area_name) {
            if (area) {
                char nameBuf[160] = {0};
                if (acc::engine::GetAreaDisplayName(area, nameBuf,
                                                   sizeof(nameBuf)) &&
                    nameBuf[0] != '\0') {
                    acc::speech::Speak(nameBuf, /*interrupt=*/false);
                    acclog::Write("MapCursor", "area name=\"%s\"", nameBuf);
                }
            }
            g_state.announced_area_name = true;
        }

        // Ensure the nav-graph topology is built for this area. Normally
        // transitions::Tick has already built it on area-enter; this
        // call is the safety net for cases where the map opens before
        // transitions has had a chance to run (e.g. map-button rebound
        // to a moment immediately after area-load) or where the wall
        // cache wasn't ready when transitions tried. BuildForArea is
        // idempotent on the same area pointer.
        if (area) {
            acc::room_topology::BuildForArea(area);
        }
    }

    // dt
    float dt = (now - g_state.last_tick_ms) * 0.001f;
    g_state.last_tick_ms = now;
    if (dt > kMaxDtSec) dt = kMaxDtSec;
    if (dt < 0.0f) dt = 0.0f;

    // Cursor delta from the player's bound movement/turn keys. Map-frame axes:
    // forward = -py (up on map = lower pixel), back = +py, turn-left = -px,
    // turn-right = +px. Pixel space y grows down per the engine's screen
    // convention. Reading the bound keys (not hardcoded WASD) keeps the map
    // cursor on the same keys the player turns/moves with in the world.
    float vx = 0.0f, vy = 0.0f;
    using acc::engine_keymap::MoveAxis;
    bool w = AxisDown(MoveAxis::Forward);
    bool s = AxisDown(MoveAxis::Backward);
    bool a = AxisDown(MoveAxis::TurnLeft);
    bool d = AxisDown(MoveAxis::TurnRight);
    if (w || s || a || d) {
        acclog::Trace("MapCursor", "input W=%d S=%d A=%d D=%d", w, s, a, d);
    }
    if (w && !s) vy -= 1.0f;
    if (s && !w) vy += 1.0f;
    if (a && !d) vx -= 1.0f;
    if (d && !a) vx += 1.0f;
    if (vx != 0.0f || vy != 0.0f) {
        // Normalise diagonals so combined keys don't 1.4× the speed.
        float mag = std::sqrt(vx * vx + vy * vy);
        vx /= mag; vy /= mag;
    }
    float step = kCursorSpeedPx * dt;
    float newPx = g_state.px + vx * step;
    float newPy = g_state.py + vy * step;

    bool clampedX = false, clampedY = false;
    if (newPx < 0.0f) { newPx = 0.0f; clampedX = true; }
    if (newPx > (float)kMapPixelMaxX) { newPx = (float)kMapPixelMaxX; clampedX = true; }
    if (newPy < 0.0f) { newPy = 0.0f; clampedY = true; }
    if (newPy > (float)kMapPixelMaxY) { newPy = (float)kMapPixelMaxY; clampedY = true; }
    bool moved = (newPx != g_state.px) || (newPy != g_state.py);

    g_state.px = newPx;
    g_state.py = newPy;
    PixelToWorld(areaMap, g_state.px, g_state.py, g_state.world.z, g_state.world);

    // Pushing into the map boundary says so once per push, not on an idle
    // clamp and not again while the push lasts.
    const bool pushingEdge = (clampedX || clampedY) && (vx != 0.0f || vy != 0.0f);
    if (pushingEdge && !g_state.at_edge) {
        acc::speech::Speak(acc::strings::Get(acc::strings::Id::MapCursorEdge),
                           /*interrupt=*/true);
        acclog::Write("MapCursor", "edge clampX=%d clampY=%d",
                      (int)clampedX, (int)clampedY);
    }
    g_state.at_edge = pushingEdge;

    // Hover-pause narration — three parallel scans:
    //   1) FindNearestExploredMapNote — CSWSWaypoint map-notes (engine-
    //      authored landmarks).
    //   2) FindNearestUserMapPin — user-placed CSWCMapPin entries
    //      (Shift+N drops). Engine quest pins are filtered out at this
    //      tier so the cursor stays on the curated "Map hint" surface.
    //   3) FindNearestShippedHint — the mod's shipped curated hints
    //      (static table rows, fog-gated).
    // Whichever is closest in pixel space wins. `hitIsPin` propagates to
    // the speak path so the right text accessor gets called; shipped
    // hints are re-identified there via IsShippedHint on the pointer.
    int   scannedCount = 0;
    int   scannedPins  = 0;
    int   scannedHints = 0;
    float bestDistWay2  = 1e30f;
    float bestDistPin2  = 1e30f;
    float bestDistHint2 = 1e30f;
    // Owned here, not inside the scan: the scan uses SEH and cannot hold an
    // object with a destructor (C2712). Folds the whole per-tick waypoint
    // dump to one "(repeated Nx — unchanged)" line while the map sits still.
    void* hitWaypoint = nullptr;
    {
        acclog::BlockLog wayDump("MapCursor.dump");
        hitWaypoint = FindNearestExploredMapNote(mapPanel, areaMap,
                                                 g_state.px, g_state.py,
                                                 &scannedCount,
                                                 &bestDistWay2,
                                                 &wayDump);
    }
    void* clientAreaForPins = nullptr;
    {
        void* serverArea = acc::engine::GetCurrentArea();
        if (serverArea) clientAreaForPins = acc::engine::GetClientArea(serverArea);
    }
    void* hitPin = FindNearestUserMapPin(clientAreaForPins, areaMap,
                                         g_state.px, g_state.py,
                                         &scannedPins, &bestDistPin2);
    const acc::map_shipped_hints::ShippedHint* hitHint =
        FindNearestShippedHint(areaMap, g_state.px, g_state.py,
                               &scannedHints, &bestDistHint2);

    void* hit      = nullptr;
    bool  hitIsPin = false;
    float bestHit2 = 1e30f;
    if (hitWaypoint) { hit = hitWaypoint; bestHit2 = bestDistWay2; }
    if (hitPin && bestDistPin2 < bestHit2) {
        hit = hitPin; hitIsPin = true; bestHit2 = bestDistPin2;
    }
    if (hitHint && bestDistHint2 < bestHit2) {
        hit = const_cast<void*>(static_cast<const void*>(hitHint));
        hitIsPin = false;
    }

    if (moved) {
        acclog::Trace("MapCursor",
                      "tick pixel=(%.1f,%.1f) world=(%.2f,%.2f) "
                      "scanned=%d pins=%d hints=%d hit=%p kind=%s",
                      g_state.px, g_state.py,
                      g_state.world.x, g_state.world.y,
                      scannedCount, scannedPins, scannedHints,
                      hit,
                      hitIsPin ? "pin"
                               : (hitHint && hit == hitHint ? "hint"
                                                            : "waypoint"));
    }

    // Resolve the terrain shape via the nav-graph topology (built once
    // on area-enter by transitions::Tick — the in-world walking adapter
    // reads from the same source, so cursor and walking agree on what
    // counts as a corridor / junction / dead-end). Cluster ids are
    // stable inside a perceptual region — no per-pixel drift inside a
    // curved corridor — and become the natural dedup key for the
    // ambient flow below.
    //
    // Fog-of-war gate applied first — never expose a shape label for a
    // cell the player hasn't revealed.
    bool cursorExplored = acc::engine::IsWorldPointExplored(areaMap, g_state.world);
    std::string shapeTextLocal;
    int  shapeSigLocal  = 0;
    int  shapeClusterId = acc::room_topology::kClusterIdNone;
    bool haveShape = false;
    if (cursorExplored) {
        void* areaForRoom = acc::engine::GetCurrentArea();
        if (areaForRoom) {
            haveShape = acc::room_topology::LookupAt(
                areaForRoom, g_state.world,
                shapeTextLocal,
                shapeSigLocal, shapeClusterId,
                /*allowDiagLog=*/false,
                /*requireWallReachable=*/false);
        }
    }

    if (hit) {
        // Cursor sits directly on an explicit map-note waypoint. This
        // overrides ambient announce (fog / room label) — the waypoint
        // is the more specific information and the user is clearly
        // pointing at it.
        //
        // Drop any in-flight ambient pending so the next time we leave
        // this waypoint the ambient timer starts fresh, not stale.
        g_state.pending_ambient_kind       = AmbientKind::None;
        g_state.pending_ambient_room_idx   = -1;
        g_state.pending_ambient_started_ms = 0;
        g_state.pending_shape_text.clear();

        if (hit == g_state.last_spoken_waypoint) {
            // Already spoken — keep silence.
            g_state.pending_note_waypoint = nullptr;
            g_state.pending_note_started_ms = 0;
        } else if (hit == g_state.pending_note_waypoint) {
            if (g_state.pending_note_started_ms != 0 &&
                now - g_state.pending_note_started_ms >= kHoverPauseMs) {
                char text[256] = {0};
                bool haveText = false;
                char tagBuf[128] = {0};
                bool haveTag = false;
                bool kindIsPin = g_state.pending_note_is_pin;
                bool kindIsHint =
                    acc::map_shipped_hints::IsShippedHint(hit);
                if (kindIsHint) {
                    // Shipped curated hint — mod-localised label from the
                    // static table; no engine object to read.
                    haveText = acc::map_shipped_hints::GetLabel(
                                   hit, text, sizeof(text)) &&
                               text[0] != '\0';
                    if (!haveText) {
                        const char* poi = acc::strings::Get(
                            acc::strings::Id::MapCursorWaypointPOI);
                        if (poi[0]) {
                            std::snprintf(text, sizeof(text), "%s", poi);
                        }
                    }
                } else if (kindIsPin) {
                    // Map-pin path — note_text CExoString @+0x100.
                    haveText = acc::engine::GetMapPinNoteText(
                                   hit, text, sizeof(text)) &&
                               text[0] != '\0';
                    if (!haveText) {
                        const char* generic = acc::strings::Get(
                            acc::strings::Id::MapPinNoText);
                        if (generic[0]) {
                            std::snprintf(text, sizeof(text), "%s", generic);
                        }
                    }
                } else {
                    // Waypoint path.
                    haveText = ReadWaypointMapNoteText(hit, text,
                                                       sizeof(text)) &&
                               text[0] != '\0';
                    haveTag = ReadWaypointTag(hit, tagBuf, sizeof(tagBuf));
                    if (!haveText) {
                        // Honest fallback per feedback_never_silence_-
                        // fallback_announcement: an enabled map-note pin
                        // with empty localised text would otherwise
                        // vanish for blind players even though the
                        // engine still renders the coloured dot.
                        const char* poi = acc::strings::Get(
                            acc::strings::Id::MapCursorWaypointPOI);
                        if (poi[0]) {
                            std::snprintf(text, sizeof(text), "%s", poi);
                        }
                    }
                }
                if (text[0] != '\0') {
                    // Append the surrounding terrain shape when we have
                    // one — landmark/POI first, shape second, separated
                    // by ". " so screen readers parse two sentences.
                    std::string combined = text;
                    if (haveShape && !shapeTextLocal.empty()) {
                        combined += ". ";
                        combined += shapeTextLocal;
                    }
                    acc::speech::Speak(combined.c_str(), /*interrupt=*/true);
                    acclog::Write("MapCursor",
                                  "speak %s=\"%s\" tag=\"%s\" "
                                  "shape=\"%s\" haveText=%d "
                                  "cursor=(%.1f,%.1f)",
                                  kindIsHint ? "hint"
                                             : (kindIsPin ? "pin" : "note"),
                                  text, haveTag ? tagBuf : "",
                                  haveShape ? shapeTextLocal.c_str() : "",
                                  (int)haveText,
                                  g_state.px, g_state.py);
                }
                g_state.last_spoken_waypoint    = hit;
                g_state.last_spoken_is_pin      = kindIsPin;
                g_state.pending_note_waypoint   = nullptr;
                g_state.pending_note_is_pin     = false;
                g_state.pending_note_started_ms = 0;
                // Crossing into a waypoint/pin resets ambient latching — if
                // the user then leaves back into the same fog/room they
                // came from, the ambient re-announces (the hover hit
                // counts as an "intervening event" that re-arms ambient).
                g_state.last_spoken_ambient_kind     = AmbientKind::None;
                g_state.last_spoken_ambient_room_idx = -1;
                g_state.last_spoken_ambient_text.clear();
            }
        } else {
            g_state.pending_note_waypoint   = hit;
            g_state.pending_note_is_pin     = hitIsPin;
            g_state.pending_note_started_ms = now;
        }
    } else {
        // Cursor not over a map note. Drop waypoint pending; classify
        // current ambient and run the unified hover-pause.
        if (g_state.pending_note_waypoint != nullptr) {
            g_state.pending_note_waypoint   = nullptr;
            g_state.pending_note_started_ms = 0;
        }
        if (g_state.last_spoken_waypoint != nullptr && moved) {
            // Left a note — re-arm so coming back to the same note
            // announces again.
            g_state.last_spoken_waypoint = nullptr;
        }

        // ---- Classify the current ambient state at the cursor.
        //
        // Order: fog-of-war wins outright (spoiler guard — never speak
        // a room/landmark name for an unrevealed cell, even though the
        // engine has both readily available). For explored cells, try
        // Tier 1 landmark (Bioware-authored map-note label, localized,
        // sparse), then Tier 2 mod-supplied friendly room name. Vanilla
        // resref-style room ids fall through to None and stay silent.
        AmbientKind currentKind        = AmbientKind::None;
        int         currentRoomIdx     = -1;
        int         currentLandmarkIdx = -1;
        char        currentLandmarkBuf[128] = {0};

        bool explored = acc::engine::IsWorldPointExplored(areaMap, g_state.world);
        if (!explored) {
            currentKind = AmbientKind::Unexplored;
        } else {
            constexpr float kLandmarkRangeM = 15.0f;
            Vector landmarkPos;
            if (acc::transitions::FindLandmarkNear(
                    g_state.world, kLandmarkRangeM,
                    currentLandmarkBuf, sizeof(currentLandmarkBuf),
                    landmarkPos, &currentLandmarkIdx) &&
                currentLandmarkBuf[0] != '\0') {
                currentKind = AmbientKind::Landmark;
            } else {
                void* area = acc::engine::GetCurrentArea();
                int roomIdx = -1;
                if (area) {
                    acc::engine::GetRoomAtIndexed(area, g_state.world, roomIdx);
                }
                if (roomIdx >= 0) {
                    currentRoomIdx = roomIdx;
                    char roomBuf[128] = {0};
                    if (area && acc::engine::GetRoomDisplayName(
                            area, roomIdx, roomBuf, sizeof(roomBuf)) &&
                        roomBuf[0] != '\0' &&
                        !acc::transitions::IsResrefStyleRoomName(roomBuf)) {
                        currentKind = AmbientKind::RoomName;
                    }
                }
            }
        }

        // Shape was already probed once at the top of the tick into
        // shapeTextLocal / shapeSigLocal / haveShape. If the higher
        // tiers (Landmark / friendly RoomName) found nothing and we
        // have a shape, promote currentKind to TerrainShape so the
        // hover-pause + speak flow handles it. When a higher tier
        // already won (Landmark, RoomName, Unexplored), we keep
        // currentKind as-is — the shape is appended at speak time as
        // a tail sentence rather than replacing the primary kind.
        if (currentKind == AmbientKind::None && haveShape) {
            currentKind = AmbientKind::TerrainShape;
        }

        // For Unexplored, the cell grid spans (fog doesn't follow any
        // partition) — collapse to -1 so the "same as last spoken"
        // comparison treats all fog cells as one category. For
        // Landmark, the flat-cache landmark index is the identity
        // (different waypoints re-announce, hovering the same waypoint
        // stays silent). For RoomName, the .lyt-room index keys the
        // identity (room display name comes from the engine's room
        // table). For TerrainShape we key off the room_topology
        // cluster id, which is stable across the cluster's footprint —
        // a curved corridor reads as one cluster even though the
        // per-position sig drifts. The kind tag disambiguates the
        // shared integer field against value collisions between
        // landmark indices, room indices, and cluster id sentinels
        // (None=-1, OpenArea=-2).
        int classifyKey;
        if (currentKind == AmbientKind::Landmark) {
            classifyKey = currentLandmarkIdx;
        } else if (currentKind == AmbientKind::RoomName) {
            classifyKey = currentRoomIdx;
        } else if (currentKind == AmbientKind::TerrainShape) {
            classifyKey = shapeClusterId;
        } else {
            classifyKey = -1;
        }

        // Resolve the text we'd speak for this ambient, eagerly. Used
        // both for the text-based dedup overlay (so adjacent rooms
        // with identical labels collapse to one announce) and as the
        // backing text for the speak path (so we don't have to re-
        // resolve it later). TerrainShape uses this frame's freshly-built
        // shapeTextLocal — what the dedup compares against, not what we
        // eventually speak (that uses the stashed pending_shape_text).
        std::string currentAmbientText = ResolveAmbientText(
            currentKind, currentRoomIdx, currentLandmarkBuf,
            shapeTextLocal.c_str());

        // Dedup overlay: text-equality OR (kind, key) equality.
        // The text path collapses adjacent regions whose labels happen
        // to be identical ("junction, North, East" in two adjacent
        // clusters) into one announce. The key path is the primary
        // identity match — cluster id for TerrainShape (stable inside
        // a perceptual region), room index for Landmark/RoomName.
        bool sameAsLastSpoken =
            currentKind == g_state.last_spoken_ambient_kind &&
            (classifyKey == g_state.last_spoken_ambient_room_idx ||
             (!currentAmbientText.empty() &&
              currentAmbientText == g_state.last_spoken_ambient_text));
        // Strict (kind, key) match for the pending side too — the old
        // TerrainShape special-case (kind-only) existed to survive
        // per-pixel sig drift inside curved corridors. With cluster id
        // that drift goes away (one cluster = one key for the whole
        // footprint), so the special-case isn't needed.
        bool sameAsPending =
            currentKind == g_state.pending_ambient_kind &&
            classifyKey == g_state.pending_ambient_room_idx;

        if (currentKind == AmbientKind::None) {
            // Nothing to say — clear pending. Also clear last_spoken so
            // the next named ambient (fog, landmark, friendly room,
            // terrain shape) gets re-armed. Pre-terrain-shape, vanilla
            // KOTOR areas spent most of their map area in None; with
            // shape probing those now resolve to TerrainShape almost
            // everywhere, so None mostly means "explored cell but wall
            // cache not built yet" or "Unexplored already announced
            // and we're inside the same fog cell".
            g_state.pending_ambient_kind         = AmbientKind::None;
            g_state.pending_ambient_room_idx     = -1;
            g_state.pending_ambient_started_ms   = 0;
            g_state.last_spoken_ambient_kind     = AmbientKind::None;
            g_state.last_spoken_ambient_room_idx = -1;
            g_state.last_spoken_ambient_text.clear();
            g_state.pending_shape_text.clear();
        } else if (sameAsLastSpoken) {
            // Already announced this exact ambient zone; stay silent
            // until something else interrupts (waypoint, different
            // room, fog↔explored flip).
            g_state.pending_ambient_kind       = AmbientKind::None;
            g_state.pending_ambient_room_idx   = -1;
            g_state.pending_ambient_started_ms = 0;
        } else if (sameAsPending) {
            // Same cluster / room as the armed timer — keep counting
            // toward the hover-pause deadline without resetting. The
            // text stashed at arm time is the cluster's stable label,
            // so no refresh is needed (room_topology builds the label
            // once per cluster at BuildForArea, not per probe).
            if (g_state.pending_ambient_started_ms != 0 &&
                now - g_state.pending_ambient_started_ms >= kHoverPauseMs) {
                // Hover-pause elapsed — resolve the text to speak. RoomName
                // re-reads GetRoomDisplayName at speak time so the buffer's
                // lifetime is tick-local. TerrainShape uses the stashed
                // pending_shape_text so the rendered label matches what
                // armed the timer, not whatever the probe sees now.
                std::string text = ResolveAmbientText(
                    currentKind, currentRoomIdx, currentLandmarkBuf,
                    g_state.pending_shape_text.c_str());
                if (!text.empty()) {
                    // Append the shape tail when the primary tier is
                    // Landmark or RoomName (the user explicitly wanted
                    // both spoken — landmark/room first, then shape).
                    // For TerrainShape pure the primary text IS the
                    // shape; for Unexplored we never expose layout.
                    std::string speakStr = text;
                    if (haveShape && !shapeTextLocal.empty() &&
                        (currentKind == AmbientKind::Landmark ||
                         currentKind == AmbientKind::RoomName)) {
                        speakStr += ". ";
                        speakStr += shapeTextLocal;
                    }
                    acc::speech::Speak(speakStr.c_str(), /*interrupt=*/true);
                    acclog::Write("MapCursor",
                                  "speak ambient kind=%s key=%d "
                                  "text=\"%s\" shape=\"%s\" "
                                  "cursor=(%.1f,%.1f)",
                                  AmbientKindStr(currentKind),
                                  classifyKey, text.c_str(),
                                  haveShape ? shapeTextLocal.c_str() : "",
                                  g_state.px, g_state.py);
                }
                g_state.last_spoken_ambient_kind     = currentKind;
                g_state.last_spoken_ambient_room_idx = classifyKey;
                g_state.last_spoken_ambient_text     = currentAmbientText;
                g_state.pending_ambient_kind         = AmbientKind::None;
                g_state.pending_ambient_room_idx     = -1;
                g_state.pending_ambient_started_ms   = 0;
                g_state.pending_shape_text.clear();
            }
        } else {
            // New ambient category (or new room within Landmark/RoomName,
            // or new TerrainShape cluster) — arm the hover-pause. For
            // TerrainShape stash the cluster's rendered description so
            // the speak path doesn't have to re-probe.
            g_state.pending_ambient_kind       = currentKind;
            g_state.pending_ambient_room_idx   = classifyKey;
            g_state.pending_ambient_started_ms = now;
            if (currentKind == AmbientKind::TerrainShape) {
                g_state.pending_shape_text = shapeTextLocal;
            } else {
                g_state.pending_shape_text.clear();
            }
        }
    }
}

}  // namespace acc::map_ui_cursor

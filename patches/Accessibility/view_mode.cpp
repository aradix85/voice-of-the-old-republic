// View mode (B): a virtual cursor walked with W/A/S/D while the character is
// frozen, which names the regions and objects it passes and takes Enter for
// the object under it. The listener follows the cursor through the
// CExoSound::SetListenerPosition detour (hooks.toml), which runs after the
// engine's own write; the camera keys keep panning natively.

#include "view_mode_internal.h"

#include "camera_announce.h"
#include "engine_area.h"
#include "engine_manager.h"
#include "engine_options.h"
#include "engine_panels.h"
#include "engine_player.h"
#include "engine_reads.h"
#include "hotkeys.h"
#include "log.h"
#include "spatial_wall_surfaces.h"
#include "speech.h"
#include "strings.h"

#include <cmath>

#pragma comment(lib, "user32.lib")

namespace acc::view_mode {

ViewModeState g_state;

namespace {

constexpr float kCursorSpeedMps = 2.0f;   // KOTOR's walk pace
constexpr float kMaxDtSec       = 0.1f;   // no jump after a stall
constexpr float kWallBackoffM   = 0.05f;  // so the next tiny step is no new crossing

void EnterViewMode() {
    // No auto-restore: view mode lasts until it is toggled off.
    if (!acc::engine::SetPlayerInputEnabled(false, /*armAutoRestore=*/false)) {
        acclog::Write("ViewMode", "enter REFUSED — SetPlayerInputEnabled(false) "
            "failed (chain unresolved or SEH); skipping toggle");
        return;
    }
    Vector pos;
    if (!acc::engine::GetPlayerPosition(pos)) {
        acc::engine::SetPlayerInputEnabled(true);
        acclog::Write("ViewMode", "enter REFUSED — player position unavailable "
            "post-disable; rolled back");
        return;
    }
    // A degenerate facing during spawn is fixed by the camera yaw next step.
    float yaw = 0.0f;
    if (!acc::engine::GetPlayerYawDegrees(yaw)) yaw = 0.0f;

    g_state = ViewModeState{};
    g_state.cursor_pos     = pos;
    g_state.cursor_yaw_deg = yaw;
    g_state.last_tick_ms   = GetTickCount();
    g_state.active         = true;
    acc::speech::Speak(acc::strings::Get(acc::strings::Id::ViewModeOn), /*interrupt=*/true);
    acclog::Write("ViewMode", "ENTER cursor=(%.2f,%.2f,%.2f) yaw=%.1f",
        pos.x, pos.y, pos.z, yaw);
}

void ExitViewMode() {
    const bool restored = acc::engine::SetPlayerInputEnabled(true);
    g_state.active = false;
    acc::speech::Speak(acc::strings::Get(acc::strings::Id::ViewModeOff), /*interrupt=*/true);
    acclog::Write("ViewMode", "EXIT restored=%d", restored ? 1 : 0);
}

// Shift+B diagnostic: a snapshot of CClientOptions in the log.
void DumpCameraStateProbe() {
    void* clientOptions = acc::engine::GetClientOptions();
    if (!clientOptions) {
        acclog::Write("ViewModeProbe", "Shift+B — GetClientOptions returned null "
            "(chain unresolved or SEH); nothing to snapshot");
        return;
    }
    uint32_t bitfield = 0, n4 = 0, nc = 0, n10 = 0, n14 = 0;
    if (!acc::engine::TryReadU32(clientOptions, kClientOptionsBitFieldOffset, &bitfield) ||
        !acc::engine::TryReadU32(clientOptions, 0x4, &n4) ||
        !acc::engine::TryReadU32(clientOptions, 0xc, &nc) ||
        !acc::engine::TryReadU32(clientOptions, 0x10, &n10) ||
        !acc::engine::TryReadU32(clientOptions, 0x14, &n14)) {
        acclog::Write("ViewModeProbe", "Shift+B — SEH fault while reading "
            "CClientOptions @%p; field-by-field dump aborted", clientOptions);
        return;
    }
    auto bit = [&](uint32_t mask) { return (bitfield & mask) != 0 ? 1 : 0; };
    acclog::Write("ViewModeProbe", "Shift+B SNAPSHOT options=%p bitfield=0x%08x "
        "[auto_level=%d mouse_look=%d autosave=%d minigame_yaxis=%d "
        "combat_movement=%d undocumented_bits=0x%08x] "
        "neighbours @+0x4=0x%08x @+0xc=0x%08x @+0x10=0x%08x "
        "@+0x14=0x%08x view_mode_active=%d",
        clientOptions, bitfield,
        bit(0x01), bit(kClientOptionsMouseLookMask), bit(0x04), bit(0x08), bit(0x10),
        bitfield & ~0x1fu, n4, nc, n10, n14, g_state.active ? 1 : 0);
}

bool KeyDown(int vk) {
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

// Move the cursor along the camera's heading (W/S) and its right (D/A), at
// walking pace, stopping short of real walls (surfaces, not portal seams).
void StepCursor(float dt) {
    const float forwardSign = (KeyDown('W') ? 1.0f : 0.0f) - (KeyDown('S') ? 1.0f : 0.0f);
    const float strafeSign  = (KeyDown('D') ? 1.0f : 0.0f) - (KeyDown('A') ? 1.0f : 0.0f);
    if (forwardSign == 0.0f && strafeSign == 0.0f) return;

    // The frozen heading until camera_announce has anchored.
    float cameraYaw = 0.0f;
    if (acc::camera_announce::TryGetCameraEngineYawDegrees(cameraYaw)) {
        g_state.cursor_yaw_deg = cameraYaw;
    }
    const float yawRad = g_state.cursor_yaw_deg * 0.017453292519943295f;
    // Right of a CCW-positive heading: the forward vector turned 90° clockwise.
    float dx = std::cos(yawRad) * forwardSign + std::sin(yawRad) * strafeSign;
    float dy = std::sin(yawRad) * forwardSign - std::cos(yawRad) * strafeSign;
    const float mag = std::sqrt(dx * dx + dy * dy);   // diagonals no faster
    if (mag > 1e-6f) {
        dx /= mag;
        dy /= mag;
    }
    const float dist = kCursorSpeedMps * dt;
    const Vector start = g_state.cursor_pos;
    const Vector end   = { start.x + dx * dist, start.y + dy * dist, start.z };

    Vector hit = end;
    if (!acc::spatial::wall_surfaces::SegmentCrossesSurface(start, end, hit)) {
        g_state.cursor_pos = end;
        g_state.at_wall = false;
        return;
    }
    // Stop just short of the hit (z unchanged: the room-flat test).
    const float hitDist = std::sqrt((hit.x - start.x) * (hit.x - start.x) +
                                    (hit.y - start.y) * (hit.y - start.y));
    const float keep = (hitDist > kWallBackoffM) ? hitDist - kWallBackoffM : 0.0f;
    if (dist > 1e-6f) {
        g_state.cursor_pos.x = start.x + dx * keep;
        g_state.cursor_pos.y = start.y + dy * keep;
    }
    if (!g_state.at_wall) {   // once per contact, not while held against it
        acc::speech::Speak(acc::strings::Get(acc::strings::Id::ViewModeWall),
                           /*interrupt=*/true);
        acclog::Write("ViewMode", "collision at (%.2f,%.2f,%.2f) cursor clamped "
            "to (%.2f,%.2f,%.2f)", hit.x, hit.y, hit.z,
            g_state.cursor_pos.x, g_state.cursor_pos.y, g_state.cursor_pos.z);
    }
    g_state.at_wall = true;
}

// Why the B toggle must wait, or nullptr. Toggling flips the engine's input
// state, and doing that under a panel that claims input left movement locked
// after the panel closed, so this gate is stricter than the interact one:
// any dialog, drilled sub-screen or modal, and a list of foreground kinds
// (every kind in IsForegroundUiBlocking belongs here as well).
const char* ToggleBlockReason() {
    if (acc::engine::HasActiveDialogPanel()) return "dialog panel in stack";
    if (acc::engine::HasActiveSubScreen()) return "sub-screen drilled";
    void* mgr = *reinterpret_cast<void**>(kAddrGuiManagerPtr);
    if (!mgr) return nullptr;
    if (*reinterpret_cast<int*>(static_cast<unsigned char*>(mgr) + kMgrModalStackSizeOffset) > 0) {
        return "modal popup up";
    }
    void* fgPanel = acc::engine::GetForegroundPanel(mgr);
    if (!fgPanel) return nullptr;
    switch (acc::engine::IdentifyPanel(fgPanel)) {
    case acc::engine::PanelKind::Container:
    case acc::engine::PanelKind::Store:
    case acc::engine::PanelKind::Examine:
    case acc::engine::PanelKind::TutorialBox:
    case acc::engine::PanelKind::MessageBoxModal:
    case acc::engine::PanelKind::StatusSummary:
    case acc::engine::PanelKind::SkillInfoBox:
    case acc::engine::PanelKind::ControllerLossBox:
    case acc::engine::PanelKind::SoloModeQuery:
    case acc::engine::PanelKind::PartySelection:
    case acc::engine::PanelKind::AreaTransition:
    case acc::engine::PanelKind::InGameMenu:
    case acc::engine::PanelKind::InGameGalaxyMap:
        return "UI panel foreground";
    default:
        return nullptr;
    }
}

}  // namespace

bool IsActive() { return g_state.active; }

bool TryGetCursorPosition(Vector& out) {
    if (!g_state.active) return false;
    out = g_state.cursor_pos;
    return true;
}

// The camera's facing in normal play and in view mode alike: the camera is
// what the user sweeps while the character stands still. The character's yaw
// only until camera_announce has anchored.
bool GetEffectiveOrientationYawDegrees(float& out) {
    float cameraYaw = 0.0f;
    if (acc::camera_announce::TryGetCameraEngineYawDegrees(cameraYaw)) {
        out = cameraYaw;
        return true;
    }
    return acc::engine::GetPlayerYawDegrees(out);
}

// In view mode the cursor, not the frozen body, is where the user "is" for
// every distance and bearing spoken.
bool GetEffectiveReferencePosition(Vector& out) {
    if (TryGetCursorPosition(out)) return true;
    return acc::engine::GetPlayerPosition(out);
}

bool GetEffectiveViewpoint(Vector& outPos, float& outYawDegrees) {
    Vector pos{};
    float  yaw = 0.0f;
    if (!GetEffectiveReferencePosition(pos)) return false;
    if (!GetEffectiveOrientationYawDegrees(yaw)) return false;
    outPos        = pos;
    outYawDegrees = yaw;
    return true;
}

void PollWin32() {
    const bool toggle = acc::hotkeys::Pressed(acc::hotkeys::Action::ViewModeToggle);
    const bool probe  = acc::hotkeys::Pressed(acc::hotkeys::Action::CameraStateProbe);
    if (!toggle && !probe) return;

    if (const char* why = ToggleBlockReason()) {
        acclog::Write("ViewMode", "B (shift=%d) blocked — %s", probe ? 1 : 0, why);
        return;
    }
    Vector playerPos;
    if (!acc::engine::GetPlayerPosition(playerPos)) {
        acclog::Write("ViewMode", "B (shift=%d) fired without player loaded; "
            "skipping", probe ? 1 : 0);
        return;
    }
    if (probe) {
        DumpCameraStateProbe();
    } else if (g_state.active) {
        ExitViewMode();
    } else {
        EnterViewMode();
    }
}

void Tick() {
    ProcessPendingDispatch();
    if (!g_state.active) return;
    // Alt-tabbed out: W/S belong to the other window. The listener stays on
    // the cursor either way.
    if (!acc::hotkeys::IsForegroundGame()) return;

    const DWORD now = GetTickCount();
    float dt = static_cast<float>(now - g_state.last_tick_ms) * 0.001f;
    g_state.last_tick_ms = now;
    if (dt > kMaxDtSec) dt = kMaxDtSec;
    if (dt < 0.0f)      dt = 0.0f;
    StepCursor(dt);

    void* area = acc::engine::GetCurrentArea();
    // The object first, so one right under the cursor wins the speech; Enter
    // last, so it reads this tick's hover.
    NarrateNearestObject(area, g_state.cursor_pos);
    AnnounceCursorRegion(area, g_state.cursor_pos);
    PollEnter();
}

}  // namespace acc::view_mode

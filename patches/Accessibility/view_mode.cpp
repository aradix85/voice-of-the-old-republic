
#include "view_mode_internal.h"

#include "camera_announce.h"
#include "engine_area.h"
#include "engine_manager.h"
#include "engine_panels.h"
#include "engine_options.h"
#include "engine_player.h"
#include "hotkeys.h"
#include "interact_dispatch.h"
#include "log.h"
#include "spatial_wall_surfaces.h"
#include "strings.h"
#include "speech.h"

namespace acc::view_mode {

namespace {

// Listener override note: the per-frame listener write lives in a detour on
// CExoSound::SetListenerPosition (hooks.toml). Doing it from this file's
// Tick would race the engine's own write in UpdateSoundEngine, which runs
// after OnUpdate. The detour substitutes cursor_pos for the engine's
// camera-derived Vector while view mode is active.
//
// No Mouse Look forcing / cursor recentring needed: the camera-rotate keys
// (Y/C under our keymap defaults) pan the camera only, the character only
// snaps on W/S commit. So freezing W/S via SetPlayerInputEnabled(false) is
// enough — the camera keys keep panning natively while W/S and the strafe
// keys (A/D) are frozen, so we can safely reuse A/D to drive the view cursor.

void EnterViewMode() {
    // armAutoRestore=false: view mode lasts until the user toggles back.
    // Default true auto-restores after 3s mid-session.
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
    g_state.cursor_pos = pos;
    float yaw = 0.0f;
    if (!acc::engine::GetPlayerYawDegrees(yaw)) {
        // Degenerate facing during spawn — first cursor step rebases on
        // camera yaw next tick.
        yaw = 0.0f;
    }
    g_state.cursor_yaw_deg     = yaw;
    g_state.last_tick_ms       = GetTickCount();
    g_state.at_wall            = false;
    g_state.hover_last_spoken     = 0;
    g_state.hover_pending         = 0;
    g_state.hover_pending_obj     = nullptr;
    g_state.hover_pending_started = 0;
    g_state.region_pending_text.clear();
    g_state.region_pending_started_ms   = 0;
    g_state.region_last_spoken_text.clear();

    g_state.active = true;
    acc::speech::Speak(acc::strings::Get(acc::strings::Id::ViewModeOn),
                /*interrupt=*/true);
    acclog::Write("ViewMode", "ENTER cursor=(%.2f,%.2f,%.2f) yaw=%.1f",
        pos.x, pos.y, pos.z, yaw);
}

void ExitViewMode() {
    bool restored = acc::engine::SetPlayerInputEnabled(true);
    g_state.active = false;
    // Engine reclaims the listener from the camera next frame; no write here.
    acc::speech::Speak(acc::strings::Get(acc::strings::Id::ViewModeOff),
                /*interrupt=*/true);
    acclog::Write("ViewMode", "EXIT restored=%d", restored ? 1 : 0);
}

void ToggleViewMode() {
    if (g_state.active) ExitViewMode();
    else                EnterViewMode();
}

// Shift+B diagnostic: snapshot CClientOptions to the log. Cheap and
// unintrusive; keeps catching state changes during unrelated RE work.
void DumpCameraStateProbe() {
    void* clientOptions = acc::engine::GetClientOptions();
    if (!clientOptions) {
        acclog::Write("ViewModeProbe", "Shift+B — GetClientOptions returned null "
            "(chain unresolved or SEH); nothing to snapshot");
        return;
    }

    unsigned int bitfield     = 0;
    unsigned int neighbour_4  = 0;
    unsigned int neighbour_c  = 0;
    unsigned int neighbour_10 = 0;
    unsigned int neighbour_14 = 0;
    bool fault = false;
    __try {
        auto* base = reinterpret_cast<unsigned char*>(clientOptions);
        bitfield     = *reinterpret_cast<unsigned int*>(base +
                          kClientOptionsBitFieldOffset);
        neighbour_4  = *reinterpret_cast<unsigned int*>(base + 0x4);
        neighbour_c  = *reinterpret_cast<unsigned int*>(base + 0xc);
        neighbour_10 = *reinterpret_cast<unsigned int*>(base + 0x10);
        neighbour_14 = *reinterpret_cast<unsigned int*>(base + 0x14);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        fault = true;
    }

    if (fault) {
        acclog::Write("ViewModeProbe", "Shift+B — SEH fault while reading "
            "CClientOptions @%p; field-by-field dump aborted",
            clientOptions);
        return;
    }

    auto bit = [&](unsigned int mask) -> int {
        return (bitfield & mask) != 0 ? 1 : 0;
    };

    acclog::Write("ViewModeProbe", "Shift+B SNAPSHOT options=%p bitfield=0x%08x "
        "[auto_level=%d mouse_look=%d autosave=%d minigame_yaxis=%d "
        "combat_movement=%d undocumented_bits=0x%08x] "
        "neighbours @+0x4=0x%08x @+0xc=0x%08x @+0x10=0x%08x "
        "@+0x14=0x%08x view_mode_active=%d",
        clientOptions, bitfield,
        bit(0x01), bit(kClientOptionsMouseLookMask),
        bit(0x04), bit(0x08), bit(0x10),
        bitfield & ~static_cast<unsigned int>(0x1f),
        neighbour_4, neighbour_c, neighbour_10, neighbour_14,
        g_state.active ? 1 : 0);
}

void StepCursor(float dt) {
    auto down = [](int vk) -> bool {
        return (GetAsyncKeyState(vk) & 0x8000) != 0;
    };

    bool w = down('W');
    bool s = down('S');
    bool d = down('D');  // strafe right
    bool a = down('A');  // strafe left
    float forwardSign = (w ? 1.0f : 0.0f) - (s ? 1.0f : 0.0f);
    float strafeSign  = (d ? 1.0f : 0.0f) - (a ? 1.0f : 0.0f);
    if (forwardSign == 0.0f && strafeSign == 0.0f) return;

    // Pre-step yaw refresh from the engine's camera reader; fall back to
    // the frozen heading until camera_announce has anchored.
    float yawDeg = g_state.cursor_yaw_deg;
    float cameraYaw = 0.0f;
    if (acc::camera_announce::TryGetCameraEngineYawDegrees(cameraYaw)) {
        yawDeg = cameraYaw;
    }
    g_state.cursor_yaw_deg = yawDeg;

    float yawRad   = yawDeg * 0.017453292519943295f;
    float forwardX = std::cos(yawRad);
    float forwardY = std::sin(yawRad);
    // Right perpendicular in CCW-positive yaw: rotate forward 90° clockwise.
    float rightX = std::sin(yawRad);
    float rightY = -std::cos(yawRad);

    float dx = forwardX * forwardSign + rightX * strafeSign;
    float dy = forwardY * forwardSign + rightY * strafeSign;
    // Normalise so diagonals don't move 1.41× faster than cardinals.
    float mag = std::sqrt(dx * dx + dy * dy);
    if (mag > 1e-6f) {
        dx /= mag;
        dy /= mag;
    }
    float dist = kCursorSpeedMps * dt;

    Vector start = g_state.cursor_pos;
    Vector end   = {
        start.x + dx * dist,
        start.y + dy * dist,
        start.z };

    // Surface-level collision: the cursor stops at real walls, not on
    // portal-seam fragments.
    Vector hit = end;
    bool collided = acc::spatial::wall_surfaces::SegmentCrossesSurface(
        start, end, hit);

    if (collided) {
        // Back off 5 cm from the hit so next tick's tiny step doesn't read
        // as a fresh crossing.
        float dx = end.x - start.x;
        float dy = end.y - start.y;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len > 1e-6f) {
            float backoff = 0.05f;
            float hx = hit.x - start.x;
            float hy = hit.y - start.y;
            float t  = std::sqrt(hx * hx + hy * hy);
            float clamped = t - backoff;
            if (clamped < 0.0f) clamped = 0.0f;
            float u = clamped / len;
            g_state.cursor_pos.x = start.x + dx * u;
            g_state.cursor_pos.y = start.y + dy * u;
            // z unchanged — room-flat assumption matches the test.
        } else {
            g_state.cursor_pos = start;  // unreachable given dist > 0 guard
        }

        // Said once per contact, not again while the cursor is held
        // against the wall.
        if (!g_state.at_wall) {
            acc::speech::Speak(acc::strings::Get(acc::strings::Id::ViewModeWall),
                               /*interrupt=*/true);
            acclog::Write("ViewMode", "collision at (%.2f,%.2f,%.2f) cursor clamped "
                "to (%.2f,%.2f,%.2f)",
                hit.x, hit.y, hit.z,
                g_state.cursor_pos.x, g_state.cursor_pos.y,
                g_state.cursor_pos.z);
        }
        g_state.at_wall = true;
    } else {
        g_state.cursor_pos = end;
        g_state.at_wall = false;
    }
}

}  // namespace

bool IsActive() { return g_state.active; }

bool TryGetCursorPosition(Vector& out) {
    if (!g_state.active) return false;
    out = g_state.cursor_pos;
    return true;
}

bool GetEffectiveOrientationYawDegrees(float& out) {
    // T2's front cone tracks where the user is *looking* — the camera
    // facing — in BOTH normal play and view mode. The camera is what the
    // user sweeps when scanning for walls/openings: in third-person the
    // character often stays put while the camera orbits (confirmed in
    // logs — the compass swept full circles while character yaw barely
    // moved). Anchoring the cone to character yaw made turning-in-place
    // produce no cone movement and no cues. Mirrors camera_announce,
    // which uses camera yaw for exactly this reason.
    float cameraYaw = 0.0f;
    if (acc::camera_announce::TryGetCameraEngineYawDegrees(cameraYaw)) {
        out = cameraYaw;
        return true;
    }
    // Camera pose not anchored yet (first ticks before camera_announce::
    // Tick() ran its anchoring pass, or camera unavailable): fall back to
    // character yaw for that tick, then switch to camera yaw.
    return acc::engine::GetPlayerYawDegrees(out);
}

bool GetEffectiveReferencePosition(Vector& out) {
    // View mode freezes the creature (SetPlayerInputEnabled(false)) and
    // hands W/S to the virtual cursor, so the cursor — not the body — is
    // where the user currently "is" for the purpose of any distance or
    // bearing we speak.
    if (TryGetCursorPosition(out)) return true;  // false unless active
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
    bool risingAlone = acc::hotkeys::Pressed(
                           acc::hotkeys::Action::ViewModeToggle);
    bool risingShift = acc::hotkeys::Pressed(
                           acc::hotkeys::Action::CameraStateProbe);
    if (!risingAlone && !risingShift) return;
    bool shift = risingShift;

    // UI-claim gate (stricter than interact_hotkey's Enter gate). Toggling
    // view mode flips engine input state; doing it while any UI claims
    // input corrupts the engine gate (post-modal-close movement lockup).
    // Scan panels[] for dialog + sub-screens (stale-Fade defeats fg-only
    // checks), modal_stack, and a foreground-kind blacklist.
    void* mgr = *reinterpret_cast<void**>(kAddrGuiManagerPtr);
    const char* blockReason = nullptr;
    if (acc::engine::HasActiveDialogPanel()) {
        blockReason = "dialog panel in stack";
    } else if (acc::engine::HasActiveSubScreen()) {
        blockReason = "sub-screen drilled";
    } else if (mgr) {
        auto* mgrBase = reinterpret_cast<unsigned char*>(mgr);
        int modalSize = *reinterpret_cast<int*>(
            mgrBase + kMgrModalStackSizeOffset);
        if (modalSize > 0) {
            blockReason = "modal popup up";
        } else {
            void* fgPanel = acc::engine::GetForegroundPanel(mgr);
            if (fgPanel) {
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
                // Same gap the interact gate had: the star map is neither a
                // drilled sub-screen nor a modal, so nothing above catches
                // it, and view mode's toggle flips engine input state — the
                // one thing this gate exists to keep away from a live panel.
                // This list and IsForegroundUiBlocking's are deliberately
                // different (this one is stricter — any modal, any drilled
                // sub-screen) but every kind in that one belongs here too;
                // add to both when a new screen appears.
                case acc::engine::PanelKind::InGameGalaxyMap:
                    blockReason = "UI panel foreground";
                    break;
                default:
                    break;
                }
            }
        }
    }
    if (blockReason) {
        acclog::Write("ViewMode", "B (shift=%d) blocked — %s",
                      shift ? 1 : 0, blockReason);
        return;
    }

    Vector playerPos;
    if (!acc::engine::GetPlayerPosition(playerPos)) {
        acclog::Write("ViewMode", "B (shift=%d) fired without player loaded; "
            "skipping", shift ? 1 : 0);
        return;
    }

    if (risingShift) {
        DumpCameraStateProbe();
        return;
    }

    if (risingAlone) {
        ToggleViewMode();
    }
}

void Tick() {
    // Before the active-gate — dispatch fires after view mode has exited.
    ProcessPendingDispatch();

    if (!g_state.active) return;

    // Foreground gate: don't consume W/S when the user is alt-tabbed out.
    // The listener detour keeps anchoring to cursor_pos either way, so the
    // soundscape stays put.
    HWND fg = GetForegroundWindow();
    if (fg) {
        DWORD pid = 0;
        GetWindowThreadProcessId(fg, &pid);
        if (pid != GetCurrentProcessId()) return;
    }

    DWORD now = GetTickCount();
    DWORD elapsedMs = now - g_state.last_tick_ms;
    g_state.last_tick_ms = now;
    float dt = static_cast<float>(elapsedMs) * 0.001f;
    if (dt > kMaxDtSec) dt = kMaxDtSec;
    if (dt < 0.0f)      dt = 0.0f;

    StepCursor(dt);

    void* area = acc::engine::GetCurrentArea();
    NarrateNearestObject(area, g_state.cursor_pos);
    // After Narrate so an object directly under the cursor still wins the
    // audio bus; both can fire in the same tick on independent completions.
    AnnounceCursorRegion(area, g_state.cursor_pos);
    // After Narrate so Enter reads the hover state computed this tick.
    PollEnter();
}

}  // namespace acc::view_mode

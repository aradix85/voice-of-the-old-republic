// Acting on the narrated target: re-announcing it, and walking, routing or pathfinding to it.

#include "cycle_input_internal.h"

#include "engine_area.h"
#include "engine_panels.h"
#include "engine_player.h"
#include "exit_memory.h"     // TargetWayInfo — where a door leads, the way there
#include "guidance_approach.h"  // ArmApproach / IsApproachInFlight — unified
                                // walk-to-act tracker (way-blocked + cancel)
#include "guidance_autowalk.h"
#include "guidance_description.h"
#include "guidance_pathfind.h"
#include "log.h"
#include "map_shipped_hints.h"  // shipped curated hints in the Map cycle
#include "narrated_target.h"
#include "strings.h"
#include "view_mode.h"      // GetEffectiveViewpoint — the frame the
                            // clock + metres are measured from
#include "speech.h"

namespace acc::cycle_input {

namespace {

// Resolve the narrated-target slot into the bundle every dash-family
// handler needs: object, server handle, current world position, current
// localised name, and current category. Returns false (and populates the
// out struct with zeroed fields) when the slot is empty or stale, in
// which case the caller speaks GuidanceNoFocus.
//
// "Current" matters here: cycle-input stamped at announce time, but the
// player may have walked since then, and the object may have moved.
// Always re-read pos / name on use so the activation payload reflects
// the world state at activation time, not stamp time.
struct NarratedActivation {
    void*    obj      = nullptr;
    uint32_t handle   = 0;
    Vector   pos      = {0.0f, 0.0f, 0.0f};
    char     name[128] = {};
    acc::filter::CycleCategory category = acc::filter::CycleCategory::Count_;
    bool     isMapPin = false;
};

// Classify a stamped object the same way cycle keys do. Returns Count_
// when the object falls outside the six locked nav categories — callers
// fall back to a generic vocabulary (default-open verb) since
// passive_narrate would already have filtered any non-nav classification
// before stamping, so we shouldn't see Count_ in practice.
acc::filter::CycleCategory ClassifyForCycle(void* obj) {
    using C = acc::filter::CycleCategory;
    for (int i = 0; i < static_cast<int>(C::Count_); ++i) {
        auto c = static_cast<C>(i);
        if (acc::filter::ObjectMatches(obj, c)) return c;
    }
    return C::Count_;
}

bool ResolveNarratedActivation(NarratedActivation& out) {
    out = {};
    acc::narrated_target::Slot slot;
    if (!acc::narrated_target::TryGet(slot)) return false;

    if (slot.isMapPin) {
        // Map-pin path: pos is frozen at stamp time (pins don't move).
        // Shipped curated hints share this slot shape and resolve their
        // label from the map_shipped_hints table; real pins resolve from
        // CSWCMapPin.note_text. Fall back to the generic placeholder when
        // missing (user-placed pins always have one assigned, but
        // defended for safety).
        out.obj      = slot.obj;
        out.handle   = 0;
        out.pos      = slot.pos;
        out.category = acc::filter::CycleCategory::Landmark;
        out.isMapPin = true;
        bool named = acc::map_shipped_hints::IsShippedHint(slot.obj)
            ? acc::map_shipped_hints::GetLabel(slot.obj, out.name,
                                               sizeof(out.name))
            : (acc::engine::GetMapPinNoteText(slot.obj, out.name,
                                              sizeof(out.name)) &&
               out.name[0] != '\0');
        if (!named) {
            const char* fallback = acc::strings::Get(
                acc::strings::Id::MapPinNoText);
            std::snprintf(out.name, sizeof(out.name), "%s", fallback);
        }
        return true;
    }

    if (!acc::engine::GetObjectPosition(slot.obj, out.pos)) return false;

    out.obj      = slot.obj;
    out.handle   = slot.handle;
    out.category = ClassifyForCycle(slot.obj);
    out.isMapPin = false;
    if (!acc::engine::GetObjectName(slot.obj, out.name, sizeof(out.name)) ||
        out.name[0] == '\0') {
        // Fall back to the localised category name when the object name
        // is empty — same pattern AnnounceCurrent uses for cycle-stepped
        // items.
        const char* fallback = acc::strings::Get(
            BindingsFor(out.category).name);
        std::snprintf(out.name, sizeof(out.name), "%s", fallback);
    }
    return true;
}

// Wraps ResolveNarratedActivation with the shared empty-slot fallback used
// by `-` / Shift+- / Ctrl+- / Alt+-. On miss, speaks GuidanceNoFocus and
// logs `<tag> -> [<msg>] (no narrated target)`. Returns true iff a target
// was resolved; callers early-return on false.
bool TryResolveOrAnnounceNoFocus(NarratedActivation& a, const char* logTag) {
    if (ResolveNarratedActivation(a)) return true;
    const char* msg = acc::strings::Get(acc::strings::Id::GuidanceNoFocus);
    acc::speech::Speak(msg, /*interrupt=*/true);
    acclog::Write("Cycle", "%s -> [%s] (no narrated target)", logTag, msg);
    return false;
}

// Per-kind pre-roll Id picker. Mirrors interact_hotkey's PreRollFor (kept
// local because that one's in an anon namespace) — the Shift+- autowalk
// speech uses the same vocabulary as Enter ("Talk to X", "Open X",
// "Pick up X") so users hear consistent verbs across the two paths.
acc::strings::Id GuidancePreRollFor(acc::filter::CycleCategory c) {
    using C = acc::filter::CycleCategory;
    using S = acc::strings::Id;
    switch (c) {
        case C::Door:       return S::FmtInteractOpen;
        case C::Npc:        return S::FmtInteractTalk;
        case C::Container:  return S::FmtInteractOpen;
        case C::Item:       return S::FmtInteractTake;
        case C::Landmark:   return S::FmtInteractOpen;
        case C::Transition: return S::FmtInteractOpen;
        case C::Count_:     break;
    }
    return S::FmtInteractOpen;
}

// True when a screen or popup owns the keyboard, so queueing a walk on the
// player creature now would be the galaxy-map Enter double-fire in another
// skin: one press drives the panel AND injects an action that surfaces later,
// after the panel is gone, in whatever the engine was doing meanwhile. The
// cycle keys reach here from two routes (the engine key hook and our own
// Win32 poll) and both gate only on "is there a world", so this is the choke
// point — the dispatchers, not the routers.
//
// The Map sub-screen is exempt on purpose: auto-walking to a
// map pin from the map screen is a shipped feature, and the map always reads
// as blocked because it drills under the InGameMenu strip.
//
// Narration-only members of the family (`,` / `.` cycling, bare `-`, the
// Ctrl+- route) are deliberately NOT gated — they queue nothing, and reading the
// world out from inside a screen is useful.
bool UiOwnsKeysForWalkDispatch(const char* keyTag) {
    if (acc::engine::HasActiveMapPanel(nullptr)) return false;
    acc::engine::UiBlockState blk;
    if (!acc::engine::IsForegroundUiBlocking(&blk)) return false;
    acclog::Write("Cycle",
                  "%s refused — UI owns input (fg=%p kind=%s reason=%d); "
                  "queueing a walk here would fire after the panel closes",
                  keyTag, blk.fgPanel,
                  acc::engine::PanelKindName(blk.fgKind),
                  static_cast<int>(blk.reason));
    return true;
}

}  // namespace

// `-` repeats the last narrated target with fresh distance + clock. Reads
// the unified narrated_target slot rather than cycle_state directly — so
// `-` after a passive-narrate announcement re-announces the passive
// target, not whatever cycle was last focused on. Re-stamps the slot so
// the activation tick refreshes (same target, but the user just renewed
// their "this is what I'm thinking about" claim).
void OnAnnounceFocus() {
    NarratedActivation a;
    if (!TryResolveOrAnnounceNoFocus(a, "- (repeat)")) return;

    // Position and yaw are read separately because they gate different
    // halves of the line: metres needs only the viewpoint position, the
    // clock needs the facing too. The distance used to be computed outside
    // any success check, from a Vector left uninitialised when the position
    // read failed — a stack-garbage metre count, spoken as fact. It is now
    // inside the guard, and a viewpoint we cannot read drops the spatial
    // tail entirely rather than emitting the "1 metre" the format clamps to.
    Vector viewPos{};
    float yaw = 0.0f;
    bool havePos = acc::view_mode::GetEffectiveReferencePosition(viewPos);
    bool haveYaw = havePos &&
                   acc::view_mode::GetEffectiveOrientationYawDegrees(yaw);
    int clock = 12;
    int metres = 0;
    if (havePos) {
        float dx = a.pos.x - viewPos.x;
        float dy = a.pos.y - viewPos.y;
        metres = static_cast<int>(std::sqrt(dx * dx + dy * dy) + 0.5f);
        if (haveYaw) clock = ClockPosition(yaw, dx, dy);
    }

    char msg[576];
    if (havePos) {
        const int len =
            FormatItemPayload(a.name, haveYaw, clock, metres, msg, sizeof(msg));
        if (!a.isMapPin && len > 0 && len < static_cast<int>(sizeof(msg))) {
            const std::string way = acc::exit_memory::TargetWayInfo(a.obj, a.pos);
            if (!way.empty()) {
                std::snprintf(msg + len, sizeof(msg) - len, ", %s", way.c_str());
            }
        }
    } else {
        std::snprintf(msg, sizeof(msg), "%s", a.name);
    }
    acc::speech::Speak(msg, /*interrupt=*/true);

    // Re-stamp the slot so the activation tick refreshes. Map pins keep
    // the same frozen pos (pins don't move); game objects re-derive pos
    // on the next TryGet via GetObjectPosition.
    if (a.isMapPin) {
        acc::narrated_target::StampMapPin(a.obj, a.pos);
    } else {
        acc::narrated_target::Stamp(a.obj, a.handle);
    }
    acclog::Write("Cycle", "- (repeat) -> [%s] obj=%p handle=0x%08x mapPin=%d",
                  msg, a.obj, a.handle, (int)a.isMapPin);
}

// Shift+- — autowalk the player to the currently-focused target. Hybrid
// dispatch:
//   - A real interactable object → `acc::guidance::UseObject`. The engine
//     walks to use-range and triggers the kind-appropriate callback (door
//     open, container loot, item pickup, NPC dialog, transition cross-load),
//     stopping at a sensible distance and facing the object.
//   - A map pin / waypoint, or any focused target without a usable object
//     handle (a non-interactable target), or an object whose USE the engine
//     refuses → `acc::guidance::WalkTo` straight to the world coordinate.
//
// The WalkTo coordinate path is new (2026-06-13). It used to be the only
// path (pre-2026-05-11) and was abandoned because AddMoveToPointAction
// "no-opped for the leader" — which we now know was a misdiagnosis: the move
// only needs the player's server AI enabled (and player input NOT disabled),
// which WalkTo now does. So the two cases that Shift+- previously refused —
// map pins (redirected to Ctrl+-) and non-interactable targets (spoke a
// failure) — now autowalk to the target's position like everything else.
//
// Empty-state: when no item is focused, speaks GuidanceNoFocus and bails.
//
// Cancel-on-second-press: when an autowalk is in flight, the next press
// cancels it via `acc::guidance::CancelMovement` (ClearAllActions, and it
// restores any AI-level WalkTo raised). Both UseObject (destHint) and WalkTo
// arm in-flight tracking, so the toggle works regardless of which path ran.
void OnPathfindFocus() {
    // Toggle-cancel branch — runs before the focus check so the user
    // can cancel with no focus selected (cycled past the end, focus
    // dropped, but they want to stop the in-flight walk).
    if (acc::guidance::IsApproachInFlight()) {
        bool ok = acc::guidance::CancelMovement();
        if (ok) {
            acc::guidance::CancelApproach();  // clear tracker (no announce)
            // Restore manual control immediately rather than waiting
            // for the 3s auto-restore — the user wants the keyboard
            // back NOW.
            acc::engine::SetPlayerInputEnabled(true);
            const char* msg = acc::strings::Get(
                acc::strings::Id::MovementCancelled);
            acc::speech::Speak(msg, /*interrupt=*/true);
            acclog::Write("Cycle", "Shift+- -> [%s] (cancel path)", msg);
            return;
        }
        // Cancel SEH-faulted — fall through to dispatch so the second
        // press at least does *something*.
        acclog::Write("Cycle", "Shift+- cancel SEH-FAULT, falling through to UseObject");
    }

    // Cancelling an in-flight walk above stays available from anywhere —
    // it only clears actions. Starting one does not: see
    // UiOwnsKeysForWalkDispatch.
    if (UiOwnsKeysForWalkDispatch("Shift+-")) return;

    NarratedActivation a;
    if (!TryResolveOrAnnounceNoFocus(a, "Shift+-")) return;

    char msg[192];
    std::snprintf(msg, sizeof(msg),
                  acc::strings::Get(GuidancePreRollFor(a.category)),
                  a.name);

    // Dispatch. Map pins and handle-less targets have no object to "use" —
    // walk straight to the coordinate. Real objects go through UseObject
    // (walk to use-range + trigger); if the engine refuses the use (a
    // non-interactable object), fall back to a plain coordinate walk so the
    // user still gets there. WalkTo manages its own AI-level + leaves player
    // input enabled, so we must NOT disable input around it; the UseObject
    // path keeps its proven input-disable.
    bool ok;
    const char* path;
    bool  inputDisabledForArm = false;
    void* armObj = nullptr;   // live-pos source for the tracker; coord walks
                              // (map pins) have no usable object → targetPos only
    // Transitions are TRIGGER regions — they fire when the PC walks *into* them,
    // not on "use". UseObject on a trigger queues a walk-to-use the engine can't
    // resolve (no use-node), so the PC never moves and the approach tracker
    // declares a false "way blocked". Route them as a coordinate walk like map
    // pins: the engine A* walks to the trigger's position and crossing into the
    // region fires the transition.
    const bool walkToCoord =
        a.isMapPin || a.handle == 0 ||
        a.category == acc::filter::CycleCategory::Transition;
    if (walkToCoord) {
        ok   = acc::guidance::WalkTo(a.pos);
        path = "WalkTo(coord)";
    } else {
        bool inputDisabled = acc::engine::SetPlayerInputEnabled(false);
        ok = acc::guidance::UseObject(a.handle);
        if (ok) {
            path = "UseObject";
            inputDisabledForArm = inputDisabled;
            armObj = a.obj;
        } else {
            if (inputDisabled) acc::engine::SetPlayerInputEnabled(true);
            ok   = acc::guidance::WalkTo(a.pos);
            path = "UseObject->WalkTo";
        }
    }

    if (ok) {
        acc::speech::Speak(msg, /*interrupt=*/true);
        // Arm the unified approach tracker (Cycle-owned, so Shift+- / W can
        // toggle-cancel it): if this walk stalls short of the target instead of
        // arriving, TickApproach announces "way blocked".
        acc::guidance::ApproachArm arm;
        arm.owner         = acc::guidance::ApproachOwner::Cycle;
        std::snprintf(arm.name, sizeof(arm.name), "%s", a.name);
        arm.targetObj     = armObj;
        arm.targetPos     = a.pos;
        arm.inputDisabled = inputDisabledForArm;
        arm.isDialog      = false;
        arm.speakBlocked  = true;
        acc::guidance::ArmApproach(arm);
        acclog::Write("Cycle", "Shift+- -> [%s] via %s obj=%p handle=0x%08x "
                      "pin=%d dest=(%.2f,%.2f,%.2f)",
                      msg, path, a.obj, a.handle, a.isMapPin ? 1 : 0,
                      a.pos.x, a.pos.y, a.pos.z);
    } else {
        char failMsg[192];
        std::snprintf(failMsg, sizeof(failMsg),
                      acc::strings::Get(acc::strings::Id::FmtGuidingFailed),
                      a.name);
        acc::speech::Speak(failMsg, /*interrupt=*/true);
        acclog::Write("Cycle", "Shift+- -> [%s] all paths FAILED (last=%s) "
                      "obj=%p handle=0x%08x pin=%d",
                      failMsg, path, a.obj, a.handle, a.isMapPin ? 1 : 0);
    }
}

// Ctrl+- — speak the route to the currently-focused object, turn by turn,
// from our own A* over the static per-area nav graph (the engine refuses
// to plot for the leader). Independent of Shift+- autowalk.
void OnRouteFocus() {
    NarratedActivation a;
    if (!TryResolveOrAnnounceNoFocus(a, "Ctrl+-")) return;

    Vector playerPos;
    if (!acc::engine::GetPlayerPosition(playerPos)) {
        // Should be impossible — caller gated on GetPlayerPosition — but
        // defend so the failure path speaks rather than going silent.
        const char* msg = acc::strings::Get(
            acc::strings::Id::GuidanceNoFocus);
        acc::speech::Speak(msg, /*interrupt=*/true);
        acclog::Write("Cycle", "Ctrl+- -> [%s] (player pos unavailable)", msg);
        return;
    }

    void* area = acc::engine::GetCurrentArea();

    std::vector<Vector> waypoints;
    acc::guidance::PathFailure failure = acc::guidance::PathFailure::None;
    bool pathOk = acc::guidance::ComputePath(area, playerPos, a.pos, waypoints,
                                             &failure);

    if (!pathOk || waypoints.empty()) {
        // An island ("no path from here, maybe from another side") is a fact
        // of the area; an unreadable map is our fault and worth reporting.
        bool unreadable = (failure == acc::guidance::PathFailure::MapUnreadable);
        char msg[256];
        std::snprintf(msg, sizeof(msg),
                      acc::strings::Get(unreadable
                          ? acc::strings::Id::FmtRouteMapUnreadable
                          : acc::strings::Id::FmtRouteNoPath),
                      a.name);
        acc::speech::Speak(msg, /*interrupt=*/true);
        acclog::Write("Cycle", "Ctrl+- -> [%s] obj=%p (ComputePath failed, "
                      "reason=%s)", msg, a.obj,
                      unreadable ? "map-unreadable" : "unreachable");
        return;
    }

    // Speak the opener ("Route to {name}") first, then the turn-by-turn
    // route description. Two separate speech calls so screen readers can
    // queue them in order without interrupt-mid-sentence behaviour. The
    // opener uses interrupt=true (preempt any in-flight speech); the
    // description uses interrupt=false so it queues behind the opener.
    char opener[192];
    std::snprintf(opener, sizeof(opener),
                  acc::strings::Get(acc::strings::Id::FmtRouteTo),
                  a.name);
    acc::speech::Speak(opener, /*interrupt=*/true);

    bool isTransition = (a.category == acc::filter::CycleCategory::Transition);
    acc::guidance::description::Speak(playerPos, waypoints, a.name,
                                      isTransition, /*interrupt=*/false);

    acclog::Write("Cycle", "Ctrl+- -> [%s] obj=%p waypoints=%zu "
                  "dest=(%.2f,%.2f,%.2f) transition=%d",
                  opener, a.obj, waypoints.size(),
                  a.pos.x, a.pos.y, a.pos.z, isTransition ? 1 : 0);
}

// Alt+- — diagnostic alternate path that bypasses the action queue via
// CSWSCreature::ForceMoveToPoint instead of AddMoveToPointAction. Same
// payload semantics (spoken announce); only the engine entry point
// differs. Used to discriminate "queue contention" from "input-mode /
// flag-bit" failure modes when WalkTo is silently dropped.
//
// Speech is identical to OnPathfindFocus — the user's audible feedback
// is the same so behaviour comparison is purely about whether the
// character actually moves. The log distinguishes via "(force path)" /
// "(queue path)" tags so post-mortem grep finds each path cleanly.
void OnPathfindFocusForce() {
    if (UiOwnsKeysForWalkDispatch("Alt+-")) return;

    NarratedActivation a;
    if (!TryResolveOrAnnounceNoFocus(a, "Alt+-")) return;

    // Alt+- is the queue-bypass ForceMoveToPoint diagnostic; for map
    // pins there's no UseObject fallback either. Speak the unsupported
    // phrase and direct the user to Ctrl+- which DOES work for pin
    // coordinates.
    if (a.isMapPin) {
        const char* msg = acc::strings::Get(
            acc::strings::Id::MapPinAltDashUnsupported);
        acc::speech::Speak(msg, /*interrupt=*/true);
        acclog::Write("Cycle", "Alt+- -> [%s] (map-pin unsupported) pin=%p",
                      msg, a.obj);
        return;
    }

    char msg[192];
    std::snprintf(msg, sizeof(msg),
                  acc::strings::Get(acc::strings::Id::FmtGuidingTo),
                  a.name);

    bool ok = acc::guidance::ForceWalkTo(a.pos);
    if (ok) {
        acc::speech::Speak(msg, /*interrupt=*/true);
        acclog::Write("Cycle", "Alt+- -> [%s] obj=%p "
                      "dest=(%.2f,%.2f,%.2f) (force path)",
                      msg, a.obj,
                      a.pos.x, a.pos.y, a.pos.z);
    } else {
        char failMsg[192];
        std::snprintf(failMsg, sizeof(failMsg),
                      acc::strings::Get(acc::strings::Id::FmtGuidingFailed),
                      a.name);
        acc::speech::Speak(failMsg, /*interrupt=*/true);
        acclog::Write("Cycle", "Alt+- -> [%s] ForceWalkTo FAILED obj=%p "
                      "dest=(%.2f,%.2f,%.2f)",
                      failMsg, a.obj, a.pos.x, a.pos.y, a.pos.z);
    }
}

void WalkToNarratedTarget() {
    OnPathfindFocus();
}

}  // namespace acc::cycle_input

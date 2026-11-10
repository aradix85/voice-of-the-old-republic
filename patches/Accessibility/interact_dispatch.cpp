// Enter / Shift+Enter on the narrated target. DispatchInteract is the one
// pipeline every caller shares (Enter, view mode, the bridge, door
// walk-through, exploration).

#include "interact_dispatch.h"
#include "interact_internal.h"

#include "door_announce.h"   // walk-through once Enter opened a door into another area
#include "endar_softlock.h"  // Trask may not try the Trask door himself
#include "engine_area.h"
#include "engine_offsets.h"
#include "engine_panels.h"
#include "engine_picker.h"
#include "engine_player.h"
#include "exit_memory.h"     // loot reminder when leaving a region
#include "filter_objects.h"
#include "guidance_approach.h"
#include "guidance_autowalk.h"
#include "log.h"
#include "narrated_target.h"
#include "speech.h"
#include "strings.h"
#include "unified_action_menu.h"

#include <cmath>
#include <cstdint>
#include <cstdio>

namespace acc::interact {

namespace {

constexpr uint32_t kActionUse  = 0x3f7;
constexpr uint32_t kActionTalk = 0x3ea;

const char* KeyName(bool forceRadial) {
    return forceRadial ? "Shift+Enter" : "Enter";
}

// Arm the approach tracker (guidance_approach) for an Enter dispatch: it
// announces "way blocked" when the walk-to-act stalls out of range and gives
// input back on a blocked use. `useHandle` lets its coordinate-walk retry
// re-fire a use on arrival; verbs with nothing to re-fire leave it 0.
void ArmInteractApproach(const char* name, void* target, bool inputDisabled,
                         bool isDialog, uint32_t useHandle = 0) {
    acc::guidance::ApproachArm arm;
    arm.owner        = acc::guidance::ApproachOwner::Interact;
    std::snprintf(arm.name, sizeof(arm.name), "%s", (name && name[0]) ? name : "?");
    arm.targetObj    = target;
    Vector p{};
    if (target && acc::engine::GetObjectPosition(target, p)) arm.targetPos = p;
    arm.inputDisabled = inputDisabled;
    arm.isDialog      = isDialog;
    arm.speakBlocked  = true;
    arm.useHandle     = useHandle;
    acc::guidance::ArmApproach(arm);
}

void SpeakNoFocus(const char* why) {
    const char* msg = acc::strings::Get(acc::strings::Id::GuidanceNoFocus);
    acc::speech::Speak(msg, /*interrupt=*/true);
    acclog::Write("Interact", "%s -> [%s]", why, msg);
}

// Speak a format string with the target name in it; `out` keeps the line for
// the log.
void SpeakWithName(acc::strings::Id fmt, const char* name, char* out, size_t cap) {
    std::snprintf(out, cap, acc::strings::Get(fmt), name);
    acc::speech::Speak(out, /*interrupt=*/true);
}

// The interact target is whatever was last spoken as a target name
// (narrated_target: passive narration, the cycle keys, view-mode hover). No
// fallback to the engine's LastTarget, which can hold something the user
// never heard. `outHandle` gets the server-side handle.
void* ResolveInteractTarget(uint32_t* outHandle) {
    *outHandle = 0;
    acc::narrated_target::Slot slot;
    if (!acc::narrated_target::TryGet(slot)) return nullptr;
    *outHandle = slot.handle;
    acclog::Write("Interact", "target source=narrated (tickStamp=%u) "
        "obj=%p handle=0x%08x",
        slot.tickStamp, slot.obj, slot.handle);
    return slot.obj;
}

// A re-press on the target already being walked to: that attempt is by
// definition not progressing (a progressing one disarms on success), and
// stacking another action on its queue only deepens the churn. Tear it down,
// queue included. A press aimed elsewhere needs nothing: the new arm
// supersedes the old one.
void ClearStaleApproach(void* target) {
    if (acc::guidance::ApproachTarget() != target) return;
    acclog::Write("Interact", "re-press on in-flight approach target=%p — "
        "clearing the stale attempt before re-dispatch", target);
    acc::guidance::CancelMovement();
    acc::guidance::CancelApproach();
}

// Which creature the action will run on, for correlating a leader swap with
// the engine action that fires.
void LogDispatchCreature() {
    void*    leader   = acc::engine::GetPlayerServerCreature();
    uint32_t leaderId = leader ? acc::engine::GetObjectHandle(leader) : 0u;
    char     leaderName[64] = "?";
    if (leader) acc::engine::GetObjectName(leader, leaderName, sizeof(leaderName));
    Vector pos{};
    if (acc::engine::GetPlayerPosition(pos)) {
        acclog::Write("Interact", "dispatch creature=%p id=0x%08x name=[%s] "
            "pos=(%.2f,%.2f,%.2f)", leader, leaderId, leaderName, pos.x, pos.y, pos.z);
    } else {
        acclog::Write("Interact", "dispatch creature=%p id=0x%08x name=[%s] pos=?",
            leader, leaderId, leaderName);
    }
}

// A map note placed at a door ("Largo's Apartment" on his door) means the
// door: a coordinate walk stops against it closed. Within the 5 m of the
// landmark-to-door label match, Enter goes to the door itself.
bool RedirectLandmarkToDoor(void* target, const char* name, bool forceRadial) {
    Vector lpos{};
    void* area = acc::engine::GetCurrentArea();
    if (!area || !acc::engine::GetObjectPosition(target, lpos)) return false;
    Vector dpos{};
    void* door = acc::engine::NearestDoor(area, lpos, 5.0f, [](void* d) {
        return !acc::engine::IsDoorCosmetic(d);
    }, &dpos);
    const uint32_t dh = door ? acc::engine::GetObjectHandle(door) : 0;
    if (!dh) return false;
    acclog::Write("Interact", "landmark [%s] sits %.1fm from door 0x%08x — Enter "
        "goes to the door", name, std::hypot(dpos.x - lpos.x, dpos.y - lpos.y), dh);
    DispatchInteract(door, dh, forceRadial);
    return true;
}

// Transitions are trigger regions that fire on walk-in and landmarks are
// waypoints: neither has a use node, so a use would never move the PC (and
// report a false "way blocked"). Walk to the coordinate instead, input left
// enabled, as Shift+- does; crossing into a transition fires it.
bool WalkToTrigger(void* target, uint32_t handle, const char* name,
                   acc::filter::CycleCategory cat, bool forceRadial) {
    const bool landmark = cat == acc::filter::CycleCategory::Landmark;
    Vector tpos{};
    if (!acc::engine::GetObjectPosition(target, tpos) || !acc::guidance::WalkTo(tpos)) {
        acclog::Write("Interact", "%s WalkTo dispatch unavailable — "
            "falling through to picker/UseObject", landmark ? "landmark" : "transition");
        return false;
    }
    char msg[192];
    SpeakWithName(PreRollFor(cat), name, msg, sizeof(msg));
    ArmInteractApproach(name, target, /*inputDisabled=*/false, /*isDialog=*/false);
    acclog::Write("Interact", "%s -> [%s] %s via WalkTo(coord) "
        "target=0x%08x pos=(%.2f,%.2f,%.2f)",
        KeyName(forceRadial), msg, landmark ? "landmark waypoint" : "transition trigger",
        handle, tpos.x, tpos.y, tpos.z);
    return true;
}

// An open door has no verb, but Enter on it means going through: walk to it.
bool WalkThroughOpenDoor(void* target, uint32_t handle, const char* name) {
    Vector dpos{};
    if (!acc::engine::GetObjectPosition(target, dpos) || !acc::guidance::WalkTo(dpos)) {
        return false;
    }
    char msg[192];
    SpeakWithName(acc::strings::Id::FmtWalkingTo, name, msg, sizeof(msg));
    ArmInteractApproach(name, target, /*inputDisabled=*/false, /*isDialog=*/false);
    acclog::Write("Interact", "Enter -> [%s] open door via WalkTo(coord) "
        "target=0x%08x pos=(%.2f,%.2f,%.2f)", msg, handle, dpos.x, dpos.y, dpos.z);
    return true;
}

// The pre-roll for a picked action. When the radial opened, the unified menu
// speaks its own rows, anchored on the descriptor's client target id (the
// param handle can be in the wrong namespace and corrupt the engine target on
// a re-anchor); with no rows it says there is nothing to choose.
void AnnouncePick(const acc::picker::ActionSnapshot& snap, uint32_t handle,
                  const char* name, acc::filter::CycleCategory cat, bool forceRadial,
                  char* msg, size_t cap) {
    if (snap.radial_opened) {
        const uint32_t anchor = snap.target_id ? snap.target_id : handle;
        if (acc::unified_menu::ArmFromRadial(name, anchor)) {
            std::snprintf(msg, cap, "action menu(%s)", name);  // the menu spoke
            return;
        }
        // Get() on each arm, not on a cached Id: a cached local once read back
        // garbage under /O2.
        const char* fmt = forceRadial
            ? acc::strings::Get(acc::strings::Id::FmtInteractNoActionsRedirect)
            : acc::strings::Get(acc::strings::Id::FmtInteractNoActions);
        std::snprintf(msg, cap, fmt, name);
        acc::speech::Speak(msg, /*interrupt=*/true);
    } else if (snap.valid && snap.label[0] != '\0') {
        std::snprintf(msg, cap, acc::strings::Get(acc::strings::Id::FmtInteractEngine),
                      snap.label, name);
        acc::speech::Speak(msg, /*interrupt=*/true);
    } else {
        SpeakWithName(PreRollFor(cat), name, msg, cap);
    }
}

// AddUseObjectAction with input disabled, its proven contract: on success
// engine_player's queue-watched session gives control back, on a stall the
// approach tracker does. A refused use gives input back at once.
bool UseWithInputDisabled(uint32_t handle, bool* inputDisabled) {
    *inputDisabled = acc::engine::SetPlayerInputEnabled(false);
    if (acc::guidance::UseObject(handle)) return true;
    if (*inputDisabled) acc::engine::SetPlayerInputEnabled(true);
    return false;
}

// Dispatch the picked verb. Use goes through AddUseObjectAction (robust over
// rough terrain), talk through ActionInitiateDialog with input enabled (the
// engine walks, then talks), and everything else, or a failed use or talk,
// through the engine's click pipeline.
bool DispatchVerb(acc::picker::ActionSnapshot& snap, uint32_t dispatchHandle,
                  const char* name, void* target, bool forceRadial) {
    if (snap.action_id == kActionUse) {
        bool inputDisabled = false;
        if (UseWithInputDisabled(dispatchHandle, &inputDisabled)) {
            acclog::Write("Interact", "use-verb dispatched via UseObject "
                "(action_id=0x%x input_disabled=%d) target=0x%08x",
                snap.action_id, inputDisabled ? 1 : 0, dispatchHandle);
            ArmInteractApproach(name, target, /*inputDisabled=*/true,
                                /*isDialog=*/false, dispatchHandle);
            return true;
        }
    }
    if (snap.action_id == kActionTalk && acc::picker::InitiateDialog(dispatchHandle)) {
        acclog::Write("Interact", "dialogue dispatched via "
            "ActionInitiateDialog target=0x%08x", dispatchHandle);
        ArmInteractApproach(name, target, /*inputDisabled=*/false, /*isDialog=*/true);
        return true;
    }
    // The second Drive re-asserts the engine target right before the click.
    if (!acc::picker::Drive(dispatchHandle, &snap, forceRadial, /*populateOnly=*/false)) {
        return false;
    }
    acclog::Write("Interact", "engine picker dispatched action_id=0x%x "
        "label=[%s] target=0x%08x", snap.action_id, snap.label, dispatchHandle);
    // Walk-to-act verbs keep input enabled, so the tracker owns no restore.
    if (acc::picker::IsWalkToActVerb(snap.action_id)) {
        ArmInteractApproach(name, target, /*inputDisabled=*/false,
                            snap.action_id == kActionTalk);
    }
    return true;
}

}  // namespace

acc::strings::Id PreRollFor(acc::filter::CycleCategory c) {
    using C = acc::filter::CycleCategory;
    using S = acc::strings::Id;
    switch (c) {
        case C::Door:       return S::FmtInteractOpen;
        case C::Npc:        return S::FmtInteractTalk;
        case C::Container:  return S::FmtInteractOpen;
        case C::Item:       return S::FmtInteractTake;
        case C::Landmark:   return S::FmtInteractOpen;  // landmark = waypoint
        case C::Transition: return S::FmtInteractOpen;  // transition = doorway
        case C::Count_:     break;
    }
    return S::FmtInteractOpen;
}

// The in-game menu (Escape menu and its tabs, which keep the InGameMenu strip
// in the foreground) is the only blocker the action-menu openers switch out
// of, as the game's own menu hotkeys do; message boxes, dialogs and stores
// stay hard blockers.
bool ShouldSwitchFromInGameMenu() {
    acc::engine::UiBlockState blk;
    if (!acc::engine::IsForegroundUiBlocking(&blk)) return false;
    return blk.fgKind == acc::engine::PanelKind::InGameMenu;
}

void OnInteract(bool forceRadial) {
    // A map pin is a destination, not an interactable: point at Ctrl+-.
    acc::narrated_target::Slot slot;
    if (acc::narrated_target::TryGet(slot) && slot.isMapPin) {
        const char* msg = acc::strings::Get(acc::strings::Id::MapPinInteractHint);
        acc::speech::Speak(msg, /*interrupt=*/true);
        acclog::Write("Interact", "%s -> [%s] (map-pin focus, no interact)",
                      KeyName(forceRadial), msg);
        return;
    }

    uint32_t handle = 0;
    void* target = ResolveInteractTarget(&handle);
    if (!target || handle == 0) {
        char why[64];
        std::snprintf(why, sizeof(why), "%s no target", KeyName(forceRadial));
        SpeakNoFocus(why);
        return;
    }
    DispatchInteract(target, handle, forceRadial);
}

void DispatchInteract(void* target, uint32_t handle, bool forceRadial) {
    using C = acc::filter::CycleCategory;
    if (!target || handle == 0) {
        SpeakNoFocus(forceRadial ? "DispatchInteract with no target (forceRadial=1)"
                                 : "DispatchInteract with no target (forceRadial=0)");
        return;
    }

    // A new interact replaces a pending walk-through (re-armed below for a
    // closed door into another area).
    acc::door_announce::DisarmWalkThrough();

    // Never skip the dispatch for a door that looks sealed: the open ATTEMPT is
    // often the story trigger (OnOpen / OnFailToOpen). On the Endar Spire,
    // end_door19's failed open runs Trask's sacrifice and end_door10_cut2's
    // open starts the tutorial battle; skipping either stranded the player.
    // Door hints belong on the engine's "This object is locked" report
    // (endar_softlock::RegisterMsgRule), which only speaks after a real refusal.
    ClearStaleApproach(target);

    char name[128] = "";
    if (!acc::engine::GetObjectName(target, name, sizeof(name)) || name[0] == '\0') {
        std::snprintf(name, sizeof(name), "%s",
                      acc::strings::Get(acc::strings::Id::CategoryItem));
    }
    const C cat = acc::filter::ClassifyObject(target);
    LogDispatchCreature();

    // The one creature-keyed refusal: Trask as leader trying his own door.
    if (acc::endar::BlockCompanionAttempt(target)) return;

    // Short reminders a beat after the walk line, never instead of it: what is
    // left to search in the region being left, and a target behind a locked
    // door the walk can never reach.
    if (!forceRadial && (cat == C::Door || cat == C::Transition)) {
        acc::exit_memory::RemindLootOnLeaving(target);
    }
    if (!forceRadial) acc::exit_memory::WarnIfBehindLock(target);

    if (cat == C::Landmark && RedirectLandmarkToDoor(target, name, forceRadial)) return;
    if ((cat == C::Transition || cat == C::Landmark) &&
        WalkToTrigger(target, handle, name, cat, forceRadial)) {
        return;
    }
    if (!forceRadial && cat == C::Door && acc::engine::IsDoorOpen(target) &&
        WalkThroughOpenDoor(target, handle, name)) {
        return;
    }
    // A closed door into another area: the press only opens it; go through once
    // it is open, so one Enter reaches the next area.
    if (!forceRadial && cat == C::Door && acc::engine::DoorHasTransitionDest(target)) {
        acc::door_announce::ArmWalkThrough(handle);
    }

    // The engine's own action picker, as on cursor hover (docs/
    // engine-action-picker.md). Populate only: the radial opens when there is
    // no default action, and the verb is dispatched below.
    acc::picker::ActionSnapshot snap = {};
    acc::picker::Drive(handle, &snap, forceRadial, /*populateOnly=*/true);
    char msg[192];
    AnnouncePick(snap, handle, name, cat, forceRadial, msg, sizeof(msg));
    acclog::Write("Interact", "%s -> [%s] target=%p handle=0x%08x cat=%s "
        "engine_label=[%s] engine_action=0x%x engine_count=%d "
        "radial_opened=%d",
        KeyName(forceRadial), msg, target, handle,
        cat == C::Count_ ? "(unclassified)" : acc::filter::CategoryName(cat),
        snap.label, snap.action_id, snap.count, snap.radial_opened ? 1 : 0);
    if (snap.radial_opened) {
        acclog::Write("Interact", "radial opened (no default action) target=0x%08x",
            handle);
        return;
    }

    // The descriptor's target id (server namespace: high bit cleared) is
    // authoritative, it follows an engine retarget to a linked object. The
    // engine writes kInvalidObjectId there when there is no target.
    const uint32_t dispatchHandle = (snap.valid && snap.target_id)
        ? (snap.target_id & ~0x80000000u)
        : handle;
    const bool handleUsable = dispatchHandle != 0u && dispatchHandle != 1u &&
                              dispatchHandle != 0xFFFFFFFFu &&
                              dispatchHandle != kInvalidObjectId;
    acclog::Write("Interact", "dispatch handle=0x%08x (param=0x%08x "
        "snap.target_id=0x%08x) usable=%d",
        dispatchHandle, handle, snap.target_id, handleUsable ? 1 : 0);

    if (snap.valid && handleUsable &&
        DispatchVerb(snap, dispatchHandle, name, target, forceRadial)) {
        return;
    }

    // No descriptor, or the engine dispatch faulted: AddUseObjectAction covers
    // the plain walk-over-and-open/talk/pick-up cases.
    if (handleUsable) {
        bool inputDisabled = false;
        if (UseWithInputDisabled(dispatchHandle, &inputDisabled)) {
            acclog::Write("Interact", "fallback UseObject dispatched "
                "(input_disabled=%d target=0x%08x) after picker returned "
                "valid=%d count=%d",
                inputDisabled ? 1 : 0, dispatchHandle, snap.valid ? 1 : 0, snap.count);
            ArmInteractApproach(name, target, /*inputDisabled=*/true,
                                /*isDialog=*/false, dispatchHandle);
            return;
        }
    }

    char failMsg[192];
    SpeakWithName(acc::strings::Id::FmtInteractFailed, name, failMsg, sizeof(failMsg));
    acclog::Write("Interact", "dispatch FAILED (handleUsable=%d dispatchHandle="
        "0x%08x) -> [%s]", handleUsable ? 1 : 0, dispatchHandle, failMsg);
}

}  // namespace acc::interact

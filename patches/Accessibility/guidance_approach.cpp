#include "guidance_approach.h"

#include <windows.h>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "engine_area.h"      // GetObjectPosition / GetObjectName
#include "engine_compass.h"   // EngineYawToCompass / CompassToSector / SectorString
#include "engine_panels.h"    // HasActiveDialogPanel / IsForegroundUiBlocking / SetGlobalDialogState
#include "engine_player.h"    // GetPlayerPosition / SetPlayerInputEnabled
#include "engine_subscreen.h" // WorldIsPaused — a paused world is not a blocked way
#include "examine_view.h"     // IsCreatureDead — corpses are not obstacles
#include "guidance_autowalk.h"// CancelMovement
#include "interact_dispatch.h" // DispatchInteract — re-fire a talk after a rescue walk
#include "log.h"
#include "prism.h"
#include "spectator_scene.h" // Endar Spire scripted spectator-battle cue
#include "strings.h"

namespace acc::guidance {

namespace {

// Unified thresholds — the interact-tuned (latest, most forgiving) set. False
// "way blocked" is worse than a silent retry, so these are generous on purpose.
constexpr DWORD kStallMs        = 1800;    // sustained no-movement = settled/blocked
constexpr DWORD kCeilingMs      = 12000;   // hard backstop (unreadable state)
constexpr float kProgressEpsSq  = 0.25f;   // (0.5m)^2 — moved threshold
constexpr float kReachedMSq     = 36.0f;   // (6m)^2 — a walk that RAN and then
                                           // stalled within this of the target =
                                           // effectively arrived / a difficult-
                                           // terrain near-miss; disarm quietly
                                           // rather than nag. Only a stall beyond
                                           // this counts as truly blocked.
constexpr float kStoodStillMSq  = 6.25f;   // (2.5m)^2 — but when the PC never took
                                           // a step, 6m is far too generous. See
                                           // the reach-tolerance note in
                                           // TickApproach.
constexpr float kReissueUseMSq  = 16.0f;   // (4m)^2 — a retried walk that settles
                                           // this close is at the object; re-fire
                                           // the use it displaced. Generous
                                           // because the walk targets the object
                                           // centre while the walkmesh can stop
                                           // the PC a metre or two short of it.
constexpr DWORD kPauseHintMs    = 1000;    // paused this long after a walk
                                           // request: say why nothing moves
constexpr DWORD kCancelGraceMs  = 300;     // movement-key cancel ignores the
                                           // first 300ms after a dispatch — covers
                                           // a key still held from before the walk
                                           // and the engine's enqueue ramp-up.

// Sidestep rescue. When even the coordinate retry leaves the PC rooted, the
// usual cause is creatures packed around it: the engine's planner will not
// start a path that begins inside their collision. 2026-11-08, Taris Upper
// City Cantina: the beaten duelists lie "dying" round Ajuur, and every Enter
// and Ctrl+Shift+N from among them ended in "way blocked" with the PC not
// taking one step; a short manual step aside always freed it.
constexpr int   kSidestepMax        = 3;     // sidesteps per arm before giving up
constexpr float kSidestepDist       = 2.0f;  // metres per sidestep
constexpr float kObstacleRadius     = 3.5f;  // creatures this close shape the step
constexpr float kInTheWayRadius     = 2.5f;  // named in the final blocked line
constexpr float kStepArrivedM       = 0.6f;  // close enough to the step point
constexpr int   kMaxObstacles       = 24;
constexpr float kRootedMaxM         = 1.5f;  // moved less than this = still rooted
constexpr float kFrontOfTargetM     = 1.8f;  // rescue walks stop this far in front
                                             // of a creature or placeable
constexpr DWORD kRefireGuardMs      = 20000; // one talk re-fire per target per 20 s

struct ApproachState {
    bool          active        = false;
    ApproachOwner owner         = ApproachOwner::Interact;
    char          name[128]     = "";
    void*         targetObj     = nullptr;
    Vector        targetPos     = {0.0f, 0.0f, 0.0f};
    bool          haveTargetPos = false;
    bool          inputDisabled = false;
    bool          isDialog      = false;
    bool          speakBlocked  = true;
    bool          barkAtArm     = false;

    DWORD         armedAt       = 0;
    bool          haveProgress  = false;
    Vector        lastPos       = {0.0f, 0.0f, 0.0f};
    DWORD         progressAt    = 0;

    // Did the walk ever physically start? Latched by the movement branch. False
    // at the stall verdict means the PC has not taken one step since the arm —
    // the dispatch never became motion, however healthy the queue looked.
    bool          movedSinceArm = false;
    // The coordinate-walk retry is one-shot per arm; without this a target the
    // A* also can't reach would re-dispatch every stall window forever.
    bool          coordRetried  = false;

    // Use-verb handle to re-fire once the retried walk lands (see the header
    // note on ApproachArm::useHandle), and its one-shot latch.
    uint32_t      useHandle     = 0;
    bool          useReissued   = false;

    // World paused (Space, or the engine's auto-pause when an enemy comes
    // into view) while armed: since when, and whether the hint was spoken.
    DWORD         pausedSince     = 0;
    bool          pauseHintSpoken = false;

    // Sidestep rescue (see kSidestepMax): how many were dispatched, whether
    // one is walking now, where to, and the directions already tried.
    int           sidestepTries = 0;
    bool          stepping      = false;
    Vector        stepPos       = {0.0f, 0.0f, 0.0f};
    float         triedYaw[kSidestepMax] = {};

    // Where the current walk (the first, the coordinate retry or a re-walk)
    // started. "Rooted" means the PC is still within kRootedMaxM of it: a
    // half-metre shuffle on a re-walk must not count as a walk that ran.
    Vector        walkStartPos  = {0.0f, 0.0f, 0.0f};
    bool          haveWalkStart = false;
};

// Last talk re-fired after a rescue walk, so a target that still cannot be
// reached does not loop dialog -> retry -> re-fire forever.
uint32_t g_refireHandle = 0;
DWORD    g_refireAt     = 0;
ApproachState g_st;

// Outcome of the last approach that ended, and the serial of the latest arm
// (see LastApproachOutcome).
uint32_t        g_armSerial = 0;
ApproachOutcome g_outcome   = ApproachOutcome::None;

void EndApproach(ApproachOutcome outcome) {
    g_st.active = false;
    g_outcome   = outcome;
}

// Live target position when the object is readable, else the stamped fallback.
// Returns false when neither is available.
bool ResolveTargetPos(Vector& out) {
    if (g_st.targetObj &&
        acc::engine::GetObjectPosition(g_st.targetObj, out)) {
        return true;
    }
    if (g_st.haveTargetPos) {
        out = g_st.targetPos;
        return true;
    }
    return false;
}

// Planar player→target gap, in metres.
float GapTo(const Vector& tgt, const Vector& pos) {
    float dx = tgt.x - pos.x;
    float dy = tgt.y - pos.y;
    return std::sqrt(dx * dx + dy * dy);
}

void MarkWalkStart() {
    Vector pc;
    g_st.haveWalkStart = acc::engine::GetPlayerPosition(pc);
    if (g_st.haveWalkStart) g_st.walkStartPos = pc;
}

// Where a rescue walk should go. A creature or placeable often stands where
// the walkmesh cannot reach: a shopkeeper behind a counter (2026-11-09, Janice
// Nall in the Taris droid shop: the walk to her exact spot never started,
// three sidesteps later still "way blocked"). Aim for a point in front of it
// on the player's side instead; from there the engine's talk or use resolves.
// Waypoints and coordinates keep their exact position.
Vector RescueDestination(const Vector& tgt) {
    if (!g_st.targetObj) return tgt;
    const int kind = acc::engine::GetObjectKind(g_st.targetObj);
    if (kind != static_cast<int>(acc::engine::GameObjectKind::Creature) &&
        kind != static_cast<int>(acc::engine::GameObjectKind::Placeable)) {
        return tgt;
    }
    Vector pc;
    if (!acc::engine::GetPlayerPosition(pc)) return tgt;
    const float dx = pc.x - tgt.x;
    const float dy = pc.y - tgt.y;
    const float d  = std::sqrt(dx * dx + dy * dy);
    if (d < kFrontOfTargetM + 0.7f) return tgt;
    return Vector{tgt.x + dx / d * kFrontOfTargetM,
                  tgt.y + dy / d * kFrontOfTargetM, tgt.z};
}

// The walk never physically started (see the header note): re-issue it as a
// plain coordinate walk to the target position rather than hand the player a
// "way blocked" they would reasonably read as "there is no route at all".
// Returns true when a retry is in flight — caller must not announce.
//
// Gated on movement, not on the action queue. Both observed shapes of a
// never-started walk look healthy in the queue: the discarded use action leaves
// it flat (0→1→0 in ~50ms), and the unroutable one drives a re-plan loop that
// thrashes 4↔6 for the whole stall window. Neither moves the PC one step.
bool TryCoordinateWalkRetry(const Vector& tgt) {
    if (g_st.movedSinceArm || g_st.coordRetried) return false;

    // Clear the failed plan first. In the re-plan-loop case the engine is still
    // churning actions that would fight the coordinate walk; in the discarded
    // case the queue is already empty and this is a no-op.
    acc::guidance::CancelMovement();

    // WalkTo manages its own AI level and needs manual input left enabled — the
    // same reason cycle_input's map-pin branch never disables it. Hand input
    // back first and drop the flag so a later OnBlocked can't restore twice.
    if (g_st.inputDisabled) {
        acc::engine::SetPlayerInputEnabled(true);
        g_st.inputDisabled = false;
    }

    g_st.coordRetried = true;
    const Vector dest = RescueDestination(tgt);
    MarkWalkStart();
    if (!acc::guidance::WalkTo(dest)) {
        acclog::Write("Approach", "walk never started; coordinate-walk retry "
            "failed to queue name=[%s] — falling through to blocked", g_st.name);
        return false;
    }

    g_st.haveProgress = false;          // restart the stall window clean
    g_st.progressAt   = GetTickCount();
    acclog::Write("Approach", "walk never started (PC has not moved since arm) "
        "— retrying as coordinate walk to (%.2f,%.2f,%.2f)%s name=[%s]",
        dest.x, dest.y, dest.z,
        (dest.x != tgt.x || dest.y != tgt.y) ? " (in front of the target)" : "",
        g_st.name);
    return true;
}

// Living creatures (party included, the leader not) within `radius` of `pc`.
// Fills positions and, when `outObj` is given, the objects. Returns the count.
int NearbyCreatures(const Vector& pc, float radius, Vector* outPos,
                    void** outObj, int maxCount) {
    void* area = acc::engine::GetCurrentArea();
    if (!area) return 0;
    void* leader = acc::engine::GetPlayerServerCreature();
    int n = 0;
    acc::engine::AreaObjectIterator it(area);
    while (void* obj = it.Next()) {
        if (n >= maxCount) break;
        if (obj == leader) continue;
        if (acc::engine::GetObjectKind(obj) !=
            static_cast<int>(acc::engine::GameObjectKind::Creature)) {
            continue;
        }
        if (acc::examine_view::IsCreatureDead(obj)) continue;
        Vector p;
        if (!acc::engine::GetObjectPosition(obj, p)) continue;
        if (GapTo(p, pc) > radius) continue;
        outPos[n] = p;
        if (outObj) outObj[n] = obj;
        ++n;
    }
    return n;
}

// Pick a sidestep point kSidestepDist from `pc`: as far from nearby creatures
// as possible, leaning toward the target, but never straight at it (that line
// is the one that just failed) and never back along an already-tried
// direction. Returns false when every direction is hemmed in.
bool ChooseSidestep(const Vector& pc, const Vector& tgt, Vector& out,
                    float& outYaw, float& outNearest) {
    Vector obs[kMaxObstacles];
    const int n = NearbyCreatures(pc, kObstacleRadius, obs, nullptr,
                                  kMaxObstacles);
    const float kPi   = 3.14159265358979f;
    const float toTgt = std::atan2(tgt.y - pc.y, tgt.x - pc.x);
    float best = -1.0e9f;
    bool  found = false;
    for (int k = 0; k < 16; ++k) {
        const float a   = k * (2.0f * kPi / 16.0f);
        const float fwd = std::cos(a - toTgt);      // 1 toward target
        if (fwd > 0.9f) continue;                   // the failed line itself
        bool tried = false;
        for (int t = 0; t < g_st.sidestepTries; ++t) {
            if (std::cos(a - g_st.triedYaw[t]) > 0.85f) { tried = true; break; }
        }
        if (tried) continue;
        Vector c{pc.x + std::cos(a) * kSidestepDist,
                 pc.y + std::sin(a) * kSidestepDist, pc.z};
        float nearest = 99.0f;
        for (int i = 0; i < n; ++i) {
            const float d = GapTo(obs[i], c);
            if (d < nearest) nearest = d;
        }
        if (nearest < 1.2f) continue;               // steps into someone
        const float score = (nearest < 3.0f ? nearest : 3.0f) + 0.75f * fwd;
        if (score > best) {
            best       = score;
            out        = c;
            outYaw     = a;
            outNearest = nearest;
            found      = true;
        }
    }
    return found;
}

// Rooted even after the coordinate retry: step aside and walk again (see
// kSidestepMax). Returns true when a sidestep is in flight.
bool TrySidestep(const Vector& tgt) {
    if (!g_st.coordRetried) return false;
    if (g_st.sidestepTries >= kSidestepMax) return false;
    Vector pc;
    if (!acc::engine::GetPlayerPosition(pc)) return false;
    if (g_st.haveWalkStart && GapTo(pc, g_st.walkStartPos) > kRootedMaxM) {
        return false;                   // the walk ran; not a crowd problem
    }
    // Only a crowd is worth stepping around. With nobody beside the PC the
    // block is the walkmesh or a closed door, and a sidestep only wastes time
    // (2026-11-09: three sidesteps at the North Apartments door and at the
    // droid shop counter, the nearest creature 3.6 m and more away).
    {
        Vector beside[kMaxObstacles];
        if (NearbyCreatures(pc, kInTheWayRadius, beside, nullptr,
                            kMaxObstacles) == 0) {
            acclog::Write("Approach", "sidestep skipped: nobody within %.1fm "
                "name=[%s]", kInTheWayRadius, g_st.name);
            return false;
        }
    }
    Vector step{};
    float  yaw = 0.0f, nearest = 0.0f;
    if (!ChooseSidestep(pc, tgt, step, yaw, nearest)) {
        acclog::Write("Approach", "sidestep: every direction hemmed in "
            "name=[%s]", g_st.name);
        return false;
    }
    acc::guidance::CancelMovement();
    if (g_st.inputDisabled) {
        acc::engine::SetPlayerInputEnabled(true);
        g_st.inputDisabled = false;
    }
    if (!acc::guidance::WalkTo(step)) {
        acclog::Write("Approach", "sidestep: WalkTo refused name=[%s]",
                      g_st.name);
        return false;
    }
    g_st.triedYaw[g_st.sidestepTries] = yaw;
    ++g_st.sidestepTries;
    g_st.stepping     = true;
    g_st.stepPos      = step;
    g_st.haveProgress = false;
    g_st.progressAt   = GetTickCount();
    acclog::Write("Approach", "sidestep %d/%d: rooted, stepping %.1fm to "
        "(%.2f,%.2f) (nearest creature there %.1fm) name=[%s]",
        g_st.sidestepTries, kSidestepMax, kSidestepDist, step.x, step.y,
        nearest, g_st.name);
    return true;
}

// The sidestep landed (or got partway): walk to the target again. The re-walk
// counts as a fresh start, so a PC that stays rooted is caught again.
void ResumeAfterSidestep() {
    g_st.stepping = false;
    Vector tgt;
    MarkWalkStart();
    if (!ResolveTargetPos(tgt) || !acc::guidance::WalkTo(RescueDestination(tgt))) {
        acclog::Write("Approach", "sidestep: re-walk could not be queued "
            "name=[%s] — disarm", g_st.name);
        EndApproach(ApproachOutcome::Other);
        return;
    }
    g_st.movedSinceArm = false;
    g_st.haveProgress  = false;
    g_st.progressAt    = GetTickCount();
    acclog::Write("Approach", "sidestep done — walking to target again "
        "(%.2f,%.2f) name=[%s]", tgt.x, tgt.y, g_st.name);
}

// Nearest living creature within kInTheWayRadius of the PC, for the final
// blocked line. Fills `name` (trailing blanks trimmed).
bool CreatureInTheWay(char* name, size_t nameSize) {
    Vector pc;
    if (!acc::engine::GetPlayerPosition(pc)) return false;
    Vector pos[kMaxObstacles];
    void*  obj[kMaxObstacles] = {};
    const int n = NearbyCreatures(pc, kInTheWayRadius, pos, obj, kMaxObstacles);
    int   bestI = -1;
    float bestD = 99.0f;
    for (int i = 0; i < n; ++i) {
        const float d = GapTo(pos[i], pc);
        if (d < bestD) { bestD = d; bestI = i; }
    }
    if (bestI < 0) return false;
    name[0] = '\0';
    if (!acc::engine::GetObjectName(obj[bestI], name, nameSize) || !name[0]) {
        return false;
    }
    size_t len = std::strlen(name);
    while (len > 0 && name[len - 1] == ' ') name[--len] = '\0';
    return len > 0;
}

// The coordinate-walk retry only walks. It has to clear the failed plan to free
// the pathfinder, so the use action it replaced is gone by the time the PC
// arrives — leaving the player standing at the object needing a second press
// (patch-20260816-105520.log: the retry lands at 1.19m, and Enter from there
// opens the console instantly). Re-fire the use once on arrival so the rescue
// completes in one press.
//
// Scoped to the retry path. A settle that never needed a retry already ran its
// action — a door bashed at arm's length — and re-firing there would act twice.
//
// Returns true when a use is in flight; the caller must not disarm. Success 1/2
// (conversation / panel) takes it from here, and if nothing surfaces the next
// stall lands back on the quiet disarm with the latch already set.
bool TryReissueUse(float gap) {
    if (!g_st.coordRetried || g_st.useReissued) return false;
    if (gap * gap > kReissueUseMSq) return false;

    // A talk that needed the rescue walk: the conversation it asked for never
    // opened, so ask again from here through the same Enter pipeline. Guarded
    // per target so an unreachable speaker cannot loop.
    if (g_st.useHandle == 0 && g_st.isDialog && g_st.targetObj) {
        const uint32_t h   = acc::engine::GetObjectHandle(g_st.targetObj);
        const DWORD    now = GetTickCount();
        if (h == 0 || (h == g_refireHandle && now - g_refireAt < kRefireGuardMs)) {
            return false;
        }
        g_st.useReissued = true;
        g_refireHandle   = h;
        g_refireAt       = now;
        void* obj = g_st.targetObj;
        acclog::Write("Approach", "arrived after rescue walk (gap %.2fm) — "
            "talking again handle=0x%08x name=[%s]", gap, h, g_st.name);
        EndApproach(ApproachOutcome::Other);
        acc::interact::DispatchInteract(obj, h, /*forceRadial=*/false);
        return true;
    }
    if (g_st.useHandle == 0) return false;

    g_st.useReissued = true;

    // Mirror interact_dispatch's 0x3f7 contract exactly: input off around the
    // use, tracker owns the force-restore if this ends up blocked. At this range
    // the engine resolves the use-node without a walk, so the freeze is
    // momentary and engine_player's queue-watched restore returns control when
    // the queue drains.
    bool disabled = acc::engine::SetPlayerInputEnabled(false);
    if (!acc::guidance::UseObject(g_st.useHandle)) {
        if (disabled) acc::engine::SetPlayerInputEnabled(true);
        acclog::Write("Approach", "arrived after retry but UseObject refused "
            "handle=0x%08x name=[%s] — falling through to quiet disarm",
            g_st.useHandle, g_st.name);
        return false;
    }
    g_st.inputDisabled = disabled;

    // Give the re-fired use a full stall window to open its panel before any
    // further verdict.
    g_st.haveProgress = false;
    g_st.progressAt   = GetTickCount();
    acclog::Write("Approach", "arrived after coordinate-walk retry (gap %.2fm) — "
        "re-firing use handle=0x%08x name=[%s]", gap, g_st.useHandle, g_st.name);
    return true;
}

// A party member standing at the target, closer to it than the player and
// within a few metres: the walk cannot get past them to the use point.
// 2026-11-08: Trask stood on the Bridge door's use point, every Enter ended
// in "way blocked" and the player could not know why. Fills `name`.
bool CompanionInTheWay(const Vector& tgt, char* name, size_t nameSize) {
    Vector pc;
    if (!acc::engine::GetPlayerPosition(pc)) return false;
    const float pcGap = GapTo(tgt, pc);
    void* leader = acc::engine::GetPlayerServerCreature();
    uint32_t handles[12] = {};
    const int count = acc::engine::GetPartyMembers(handles, 12);
    for (int i = 0; i < count; ++i) {
        void* obj = acc::engine::ResolveServerObjectHandle(handles[i]);
        if (!obj || obj == leader) continue;
        Vector p;
        if (!acc::engine::GetObjectPosition(obj, p)) continue;
        const float gap = GapTo(tgt, p);
        if (gap > 3.0f || gap >= pcGap) continue;
        name[0] = '\0';
        if (acc::engine::GetObjectName(obj, name, nameSize) && name[0]) {
            return true;
        }
    }
    return false;
}

// Break a blocked approach: cancel the bouncing walk, restore input if the
// caller disabled it (explicit, so engine_player's queue-watched restore and
// this tracker never fight), clear dialog-pending limbo (talk only), and
// announce "way blocked" when the arm asked for it. Disarms.
void OnBlocked(DWORD stalledMs) {
    acc::guidance::CancelMovement();
    if (g_st.inputDisabled) {
        acc::engine::SetPlayerInputEnabled(true);
    }
    if (g_st.isDialog) {
        acc::engine::SetGlobalDialogState(0);
    }

    if (g_st.speakBlocked) {
        // Scoped Endar Spire spectator battle: a walk toward one of the
        // doomed Republic soldiers can never land (walkmesh gap). Swap the
        // generic "way blocked" for the dramatic in-world line so the repeat
        // reinforces "you can't reach them, press on to the bridge".
        if (acc::spectator::IsScriptedBattleSoldier(g_st.targetObj)) {
            prism::Speak(acc::spectator::DramaticLine(), /*interrupt=*/true);
            acclog::Write("Approach",
                "blocked on scripted-battle soldier -> [%s]",
                acc::spectator::DramaticLine());
        } else {
            Vector tgt;
            char msg[192];
            char who[64];
            if (ResolveTargetPos(tgt) && CompanionInTheWay(tgt, who, sizeof(who))) {
                std::snprintf(msg, sizeof(msg),
                              acc::strings::Get(acc::strings::Id::FmtCompanionInTheWay),
                              who);
                prism::Speak(msg, /*interrupt=*/true);
                acclog::Write("Approach", "blocked with %s at the target -> [%s]",
                              who, msg);
            } else if (!g_st.movedSinceArm &&
                       CreatureInTheWay(who, sizeof(who))) {
                char tail[128];
                std::snprintf(tail, sizeof(tail),
                              acc::strings::Get(acc::strings::Id::FmtCreatureInTheWay),
                              who);
                std::snprintf(msg, sizeof(msg), "%s. %s",
                              acc::strings::Get(acc::strings::Id::InteractWayBlocked),
                              tail);
                prism::Speak(msg, /*interrupt=*/true);
                acclog::Write("Approach", "blocked with %s beside the PC -> [%s]",
                              who, msg);
            } else if (ResolveTargetPos(tgt) && g_st.name[0]) {
                SpeakWayBlocked(g_st.name, tgt);
            } else {
                std::snprintf(msg, sizeof(msg), "%s",
                              acc::strings::Get(acc::strings::Id::InteractWayBlocked));
                prism::Speak(msg, /*interrupt=*/true);
            }
        }
    }

    acclog::Write("Approach", "BLOCKED — owner=%d isDialog=%d inputDisabled=%d "
        "stalled=%lums name=[%s] (cancelled approach)",
        static_cast<int>(g_st.owner), g_st.isDialog ? 1 : 0,
        g_st.inputDisabled ? 1 : 0, static_cast<unsigned long>(stalledMs),
        g_st.name);
    EndApproach(ApproachOutcome::Blocked);
}

}  // namespace

void ArmApproach(const ApproachArm& arm) {
    DWORD now = GetTickCount();
    g_st = {};
    g_st.active        = true;
    ++g_armSerial;
    g_outcome          = ApproachOutcome::None;
    g_st.owner         = arm.owner;
    std::snprintf(g_st.name, sizeof(g_st.name), "%s",
                  (arm.name[0]) ? arm.name : "?");
    g_st.targetObj     = arm.targetObj;
    g_st.targetPos     = arm.targetPos;
    g_st.haveTargetPos = (arm.targetPos.x != 0.0f || arm.targetPos.y != 0.0f ||
                          arm.targetPos.z != 0.0f);
    g_st.inputDisabled = arm.inputDisabled;
    g_st.isDialog      = arm.isDialog;
    g_st.speakBlocked  = arm.speakBlocked;
    g_st.useHandle     = arm.useHandle;
    // Snapshot any bark already showing so a lingering ambient bubble can't
    // instantly disarm a fresh walk; only a bark that *surfaces* after arm
    // counts as this interaction's output.
    g_st.barkAtArm     = acc::engine::HasActiveBarkBubble();
    g_st.armedAt       = now;
    g_st.haveProgress  = false;
    g_st.progressAt    = now;
    MarkWalkStart();

    // If no fallback pos was supplied but we have the object, snapshot a live
    // read now so a later announce still has a position even if the object
    // becomes unreadable mid-walk (targets are stationary).
    if (!g_st.haveTargetPos && g_st.targetObj) {
        Vector p;
        if (acc::engine::GetObjectPosition(g_st.targetObj, p)) {
            g_st.targetPos     = p;
            g_st.haveTargetPos = true;
        }
    }

    acclog::Write("Approach", "armed — owner=%d isDialog=%d inputDisabled=%d "
        "speakBlocked=%d targetObj=%p useHandle=0x%08x name=[%s]",
        static_cast<int>(arm.owner), arm.isDialog ? 1 : 0,
        arm.inputDisabled ? 1 : 0, arm.speakBlocked ? 1 : 0,
        arm.targetObj, arm.useHandle, g_st.name);
}

void TickApproach() {
    if (!g_st.active) return;
    DWORD now = GetTickCount();

    // Success 1 — a conversation opened (talk verbs). PC reached range and the
    // dialog started; nothing to cancel.
    if (acc::engine::HasActiveDialogPanel()) {
        acclog::Write("Approach", "conversation open — disarm (walk-to-talk OK)");
        EndApproach(ApproachOutcome::Other);
        return;
    }

    // Success 2 — an interaction panel opened (container loot puts a blocking
    // modal in the foreground). The tracker only ever arms in-world with nothing
    // blocking, so a blocker appearing now is the interaction result.
    acc::engine::UiBlockState blk;
    if (acc::engine::IsForegroundUiBlocking(&blk)) {
        acclog::Write("Approach", "interaction panel open (fgKind=%d) — disarm "
            "(walk-to-use OK)", static_cast<int>(blk.fgKind));
        EndApproach(ApproachOutcome::Other);
        return;
    }

    // Success 3 — the use-script delivered its result as a bark bubble rather
    // than a conversation (e.g. examining an off-walkmesh placeable: the
    // hovering swoop bikes). The bark surfacing proves the interaction fired
    // even though the body never physically arrived. Disarm quietly — never
    // CancelMovement (it would ClearAllActions and kill any queued follow-up)
    // and never announce "way blocked". engine_player's queue-watched restore
    // re-enables input on its own when the queue drains/stalls.
    if (!g_st.barkAtArm && acc::engine::HasActiveBarkBubble()) {
        acclog::Write("Approach", "bark surfaced — disarm (interaction fired)");
        EndApproach(ApproachOutcome::Other);
        return;
    }

    // A paused world moves nobody, so nothing here is blocked: hold the stall
    // and ceiling clocks (the queued walk resumes by itself on unpause) and
    // say once why the walk is not happening. Before this, the engine's
    // enemy-sighted auto-pause turned every walk into "way blocked" and the
    // player had no way to tell (2026-11-08, Endar Spire battle corridor).
    if (acc::engine::WorldIsPaused()) {
        if (g_st.pausedSince == 0) g_st.pausedSince = now;
        g_st.progressAt = now;
        g_st.armedAt    = now;
        if (!g_st.pauseHintSpoken && now - g_st.pausedSince >= kPauseHintMs) {
            g_st.pauseHintSpoken = true;
            const char* hint =
                acc::strings::Get(acc::strings::Id::GuidancePausedHint);
            prism::Speak(hint, /*interrupt=*/false);
            acclog::Write("Approach", "world paused while armed -> [%s] name=[%s]",
                          hint, g_st.name);
        }
        return;
    }
    g_st.pausedSince = 0;

    Vector pos;
    if (!acc::engine::GetPlayerPosition(pos)) {
        // Blind read — don't act on it; fall back to the hard ceiling so a
        // wedged unreadable state can't keep us armed forever.
        if (now - g_st.armedAt >= kCeilingMs) {
            acclog::Write("Approach", "ceiling (position unreadable) — disarm");
            EndApproach(ApproachOutcome::Other);
        }
        return;
    }

    // A sidestep that reached its point hands straight back to the target walk.
    if (g_st.stepping && GapTo(g_st.stepPos, pos) <= kStepArrivedM) {
        ResumeAfterSidestep();
        return;
    }

    // Movement liveness. lastPos updates only when the PC has moved ≥0.5 m, so a
    // normal sub-threshold stride still registers progress every few ticks and
    // keeps resetting the stall timer — a long cross-terrain walk is never cut
    // off. Grace covers the post-dispatch ramp-up before the engine enqueues.
    if (!g_st.haveProgress) {
        g_st.lastPos      = pos;
        g_st.progressAt   = now;
        g_st.haveProgress = true;
        return;
    }
    float dx = pos.x - g_st.lastPos.x;
    float dy = pos.y - g_st.lastPos.y;
    if ((dx * dx + dy * dy) >= kProgressEpsSq) {
        g_st.lastPos       = pos;
        g_st.progressAt    = now;
        g_st.movedSinceArm = true;
        return;
    }
    if (now - g_st.progressAt < kStallMs) return;          // brief pause / still
                                                           // ramping up (the
                                                           // stall window > any
                                                           // engine start latency)

    // A sidestep stalled short of its point: if it moved at all, try the
    // target again from here; if it never moved, try another direction.
    if (g_st.stepping) {
        if (g_st.movedSinceArm) {
            ResumeAfterSidestep();
            return;
        }
        g_st.stepping = false;
        Vector t2;
        if (ResolveTargetPos(t2) && TrySidestep(t2)) return;
        OnBlocked(now - g_st.progressAt);
        return;
    }

    // Stalled with no success surfaced. Either it effectively arrived (a no-panel
    // act — door/bash/mine — or a near-miss left it close: don't nag), or it
    // never got near the target (genuinely blocked). Distinguish by the live gap.
    //
    // First: if we can't resolve the target position at all, we cannot prove the
    // PC is out of range — so we must NOT cancel the (possibly still-running)
    // interaction or announce a false "way blocked". The pre-unify watchdog hit
    // exactly this: a wrong-namespace targetObj faulted GetObjectPosition and the
    // (0,0,0) fallback pos read as "miles away" → bogus "Bewegung abgebrochen"
    // mid-open (patch-20260618-223250.log, a stale build). The unify already
    // snapshots a real pos at arm so the common path resolves; this keeps the
    // blocked verdict honest for any caller whose target can't be positioned —
    // disarm quietly and let engine_player's queue-watched restore re-enable
    // input. (Mirrors the bark branch: when in doubt the interaction fired.)
    Vector tgt;
    if (!ResolveTargetPos(tgt)) {
        acclog::Write("Approach", "stalled but target position unresolvable — "
            "disarm quietly (cannot prove out of reach; not cancelling)");
        EndApproach(ApproachOutcome::Other);
        return;
    }
    // Reach tolerance depends on whether the walk ever physically ran — two very
    // different stalls otherwise wear the same distance.
    //
    // Walk RAN, then stalled: the engine got the PC moving and parked it wherever
    // the walkmesh allowed. Six metres of slack is right — the act very likely
    // fired (door/bash/mine open no panel) and nagging is worse than silence.
    //
    // Walk NEVER RAN (not one step since the arm): six metres is a lie. Observed
    // on the Telos comm console (patch-20260816-095834.log): nine Enter presses
    // from 4.5–5.5m out, every dispatch accepted, every one driving the engine's
    // 5↔6 re-plan churn with the PC rooted — and every one silently absorbed here
    // as "within reach" because it sat inside the 6m radius. The retry below
    // never got a turn and the player got no cue at all. At a standstill the only
    // honest reading of a small gap is "we were already at it and the act fired in
    // place", which is a use-range distance, not six metres. Keeping this branch
    // tight also protects those in-place acts: bashing a door at arm's length must
    // stay a quiet disarm and never be torn down by the retry's CancelMovement.
    const float gap     = GapTo(tgt, pos);
    const float reachSq = g_st.movedSinceArm ? kReachedMSq : kStoodStillMSq;
    if (gap * gap <= reachSq) {
        // Arrived after a coordinate-walk retry — the use it displaced still
        // owes the player its result. Fire it before calling this settled.
        if (TryReissueUse(gap)) return;
        acclog::Write("Approach", "settled within reach (stalled %lums, gap %.2fm, "
            "moved=%d) — disarm, no nag",
            static_cast<unsigned long>(now - g_st.progressAt), gap,
            g_st.movedSinceArm ? 1 : 0);
        EndApproach(ApproachOutcome::Arrived);
        return;
    }
    // Out of reach and stalled. Before calling it blocked, check whether the
    // walk ever actually started: a use dispatch the engine accepted but never
    // turned into motion leaves the PC rooted where it stood, and announcing
    // "way blocked" there tells the player no route exists when the coordinate
    // A* would have walked them straight to it.
    if (TryCoordinateWalkRetry(tgt)) return;
    if (TrySidestep(tgt)) return;

    OnBlocked(now - g_st.progressAt);
}

bool IsApproachInFlight() {
    return g_st.active && g_st.owner == ApproachOwner::Cycle;
}

void CancelApproach() {
    if (g_st.active) EndApproach(ApproachOutcome::Cancelled);
}

ApproachOutcome LastApproachOutcome(uint32_t* serial) {
    if (serial) *serial = g_armSerial;
    return g_st.active ? ApproachOutcome::None : g_outcome;
}

void* ApproachTarget() {
    return g_st.active ? g_st.targetObj : nullptr;
}

bool IsAnyModApproachInFlight() {
    return g_st.active;
}

bool CancelByMovement() {
    if (!g_st.active) return false;
    // Arm grace: a movement key still down from before the dispatch (or the
    // engine's post-dispatch enqueue ramp-up) must not cancel the walk before
    // it has a chance to start.
    if (GetTickCount() - g_st.armedAt < kCancelGraceMs) return false;

    acc::guidance::CancelMovement();
    if (g_st.inputDisabled) {
        acc::engine::SetPlayerInputEnabled(true);
    }
    if (g_st.isDialog) {
        acc::engine::SetGlobalDialogState(0);
    }
    const char* msg = acc::strings::Get(acc::strings::Id::MovementCancelled);
    prism::Speak(msg, /*interrupt=*/true);
    acclog::Write("Approach", "movement-cancel — owner=%d isDialog=%d "
        "inputDisabled=%d name=[%s] (manual control reclaimed)",
        static_cast<int>(g_st.owner), g_st.isDialog ? 1 : 0,
        g_st.inputDisabled ? 1 : 0, g_st.name);
    EndApproach(ApproachOutcome::Cancelled);
    return true;
}

void SpeakWayBlocked(const char* name, const Vector& targetPos) {
    char msg[192];
    bool built = false;
    if (name && name[0]) {
        Vector playerPos;
        if (acc::engine::GetPlayerPosition(playerPos)) {
            float dx = targetPos.x - playerPos.x;
            float dy = targetPos.y - playerPos.y;
            int metres = static_cast<int>(std::sqrt(dx * dx + dy * dy) + 0.5f);
            if (metres < 1) metres = 1;

            // Absolute 8-point compass of the player→target vector — same frame
            // the route readout and passive cue use (+X=East, +Y=North).
            float engineYaw = std::atan2(dy, dx) * (180.0f / 3.14159265358979f);
            int   sector    = acc::engine::CompassToSector(
                                  acc::engine::EngineYawToCompass(engineYaw));
            const char* dir = acc::strings::Get(acc::engine::SectorString(sector));

            std::snprintf(msg, sizeof(msg),
                acc::strings::Get(acc::strings::Id::FmtInteractWayBlockedTarget),
                name, metres, dir);
            built = true;
        }
    }
    if (!built) {
        std::snprintf(msg, sizeof(msg), "%s",
                      acc::strings::Get(acc::strings::Id::InteractWayBlocked));
    }
    prism::Speak(msg, /*interrupt=*/true);
    acclog::Write("Approach", "way blocked -> [%s]", msg);
}

}  // namespace acc::guidance

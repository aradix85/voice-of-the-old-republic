// Rescuing a stalled approach: the coordinate retry, the sidestep round creatures in the way, the
// re-issued use, and the "way blocked" message.

#include "guidance_approach_internal.h"

#include "engine_area.h"      // GetObjectPosition / GetObjectName
#include "engine_compass.h"   // EngineYawToCompass / CompassToSector / SectorString
#include "engine_panels.h"    // HasActiveDialogPanel / IsForegroundUiBlocking / SetGlobalDialogState
#include "engine_player.h"    // GetPlayerPosition / SetPlayerInputEnabled
#include "examine_view.h"     // IsCreatureDead — corpses are not obstacles
#include "guidance_autowalk.h"// CancelMovement
#include "interact_dispatch.h" // DispatchInteract — re-fire a talk after a rescue walk
#include "log.h"
#include "speech.h"
#include "spectator_scene.h" // Endar Spire scripted spectator-battle cue
#include "strings.h"

namespace acc::guidance {

namespace {

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

}  // namespace

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
            acc::speech::Speak(acc::spectator::DramaticLine(), /*interrupt=*/true);
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
                acc::speech::Speak(msg, /*interrupt=*/true);
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
                acc::speech::Speak(msg, /*interrupt=*/true);
                acclog::Write("Approach", "blocked with %s beside the PC -> [%s]",
                              who, msg);
            } else if (ResolveTargetPos(tgt) && g_st.name[0]) {
                SpeakWayBlocked(g_st.name, tgt);
            } else {
                std::snprintf(msg, sizeof(msg), "%s",
                              acc::strings::Get(acc::strings::Id::InteractWayBlocked));
                acc::speech::Speak(msg, /*interrupt=*/true);
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
    acc::speech::Speak(msg, /*interrupt=*/true);
    acclog::Write("Approach", "way blocked -> [%s]", msg);
}

}  // namespace acc::guidance

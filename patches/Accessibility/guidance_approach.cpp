// The unified walk-to-act approach tracker: arming it, the per-tick verdict
// (a conversation, panel or bark proved the act; the walk arrived; it
// stalled), cancelling it, and the queries.

#include "guidance_approach_internal.h"

#include "engine_area.h"
#include "engine_compass.h"
#include "engine_panels.h"
#include "engine_player.h"
#include "engine_subscreen.h"
#include "guidance_autowalk.h"
#include "log.h"
#include "speech.h"
#include "strings.h"

namespace acc::guidance {

namespace {

// Generous on purpose: a false "way blocked" is worse than a silent retry.
constexpr DWORD kStallMs        = 1800;    // sustained no-movement = settled/blocked
constexpr DWORD kCeilingMs      = 12000;   // hard backstop (unreadable state)
constexpr float kProgressEpsSq  = 0.25f;   // (0.5m)^2 — moved threshold
// Reach tolerances at a stall (see TickApproach): 6 m after a walk that ran,
// 2.5 m when the PC never took a step.
constexpr float kReachedMSq     = 36.0f;
constexpr float kStoodStillMSq  = 6.25f;
constexpr DWORD kPauseHintMs    = 1000;    // paused this long after a walk
                                           // request: say why nothing moves
// The movement-key cancel ignores the first 300 ms after a dispatch: a key
// still held from before the walk, and the engine's enqueue ramp-up.
constexpr DWORD kCancelGraceMs  = 300;
constexpr float kStepArrivedM   = 0.6f;    // close enough to a sidestep point

// Outcome of the last approach that ended, and the serial of the latest arm
// (see LastApproachOutcome).
uint32_t        g_armSerial = 0;
ApproachOutcome g_outcome   = ApproachOutcome::None;

}  // namespace

ApproachState g_st;

void EndApproach(ApproachOutcome outcome) {
    g_st.active = false;
    g_outcome   = outcome;
}

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

void MarkWalkStart() {
    Vector pc;
    g_st.haveWalkStart = acc::engine::GetPlayerPosition(pc);
    if (g_st.haveWalkStart) g_st.walkStartPos = pc;
}

void RestartStallWindow() {
    g_st.haveProgress = false;
    g_st.progressAt   = GetTickCount();
}

void ReleaseApproach() {
    acc::guidance::CancelMovement();
    if (g_st.inputDisabled) {
        acc::engine::SetPlayerInputEnabled(true);
    }
    if (g_st.isDialog) {
        acc::engine::SetGlobalDialogState(0);
    }
}

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
    g_st.leavesArea    = arm.leavesArea;
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

    // A paused world (Space, or the engine's auto-pause when an enemy comes
    // into view) moves nobody, so nothing is blocked: hold the stall and
    // ceiling clocks (the queued walk resumes on unpause) and say once why
    // the walk is not happening.
    if (acc::engine::WorldIsPaused()) {
        if (g_st.pausedSince == 0) g_st.pausedSince = now;
        g_st.progressAt = now;
        g_st.armedAt    = now;
        if (!g_st.pauseHintSpoken && now - g_st.pausedSince >= kPauseHintMs) {
            g_st.pauseHintSpoken = true;
            const char* hint =
                acc::strings::Get(acc::strings::Id::GuidancePausedHint);
            acc::speech::Speak(hint, /*interrupt=*/false);
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

    if (TickPartyHold(pos, now)) return;

    // A sidestep that reached its point hands straight back to the target walk.
    if (g_st.stepping && DistXY(g_st.stepPos, pos) <= kStepArrivedM) {
        ResumeAfterSidestep();
        return;
    }

    // Movement liveness. lastPos updates only when the PC has moved ≥0.5 m, so a
    // normal sub-threshold stride still registers progress every few ticks and
    // keeps resetting the stall timer — a long cross-terrain walk is never cut
    // off. The stall window outlasts the engine's start latency.
    if (!g_st.haveProgress) {
        g_st.lastPos      = pos;
        g_st.progressAt   = now;
        g_st.haveProgress = true;
        return;
    }
    if (DistXYSq(pos, g_st.lastPos) >= kProgressEpsSq) {
        g_st.lastPos       = pos;
        g_st.progressAt    = now;
        g_st.movedSinceArm = true;
        return;
    }
    if (now - g_st.progressAt < kStallMs) return;

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
        if (TryRegionWalk()) return;
        OnBlocked(now - g_st.progressAt);
        return;
    }

    // Stalled with no success surfaced: either it effectively arrived (a
    // no-panel act such as a door, bash or mine, or a near miss) or it never
    // got near the target. Without a target position the PC can't be proved
    // out of range, so it disarms quietly instead of cancelling a possibly
    // running interaction or announcing a false "way blocked".
    Vector tgt;
    if (!ResolveTargetPos(tgt)) {
        acclog::Write("Approach", "stalled but target position unresolvable — "
            "disarm quietly (cannot prove out of reach; not cancelling)");
        EndApproach(ApproachOutcome::Other);
        return;
    }
    // A walk that RAN and then stalled was parked wherever the walkmesh
    // allowed; the act very likely fired, and six metres of slack beats
    // nagging. A walk that NEVER RAN only counts as arrived at use range: at a
    // standstill a small gap means the act fired in place (a door bashed at
    // arm's length must stay a quiet disarm), and six metres would swallow
    // every rooted press without a cue.
    const float gap     = DistXY(tgt, pos);
    const float reachSq = g_st.movedSinceArm ? kReachedMSq : kStoodStillMSq;
    if (gap * gap <= reachSq) {
        // Arrived after a coordinate-walk retry — the use it displaced still
        // owes the player its result. Fire it before calling this settled.
        if (TryReissueUse(gap)) return;
        // Settled, yet the use is still queued: the use point is out of
        // reach from this side. Go to where the floor comes nearest to the
        // target, and say so when even that leaves the use undone.
        if (TryFloorNearestTarget(tgt)) return;
        if (UseNeverLanded()) {
            OnBlocked(now - g_st.progressAt);
            return;
        }
        acclog::Write("Approach", "settled within reach (stalled %lums, gap %.2fm, "
            "moved=%d) — disarm, no nag",
            static_cast<unsigned long>(now - g_st.progressAt), gap,
            g_st.movedSinceArm ? 1 : 0);
        EndApproach(ApproachOutcome::Arrived);
        return;
    }
    // Out of reach and stalled. A walk that never started gets a coordinate
    // retry and then sidesteps before anything calls it blocked.
    if (TryCoordinateWalkRetry(tgt)) return;
    if (TrySidestep(tgt)) return;
    if (TryRegionWalk()) return;

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
    if (GetTickCount() - g_st.armedAt < kCancelGraceMs) return false;

    ReleaseApproach();
    const char* msg = acc::strings::Get(acc::strings::Id::MovementCancelled);
    acc::speech::Speak(msg, /*interrupt=*/true);
    acclog::Write("Approach", "movement-cancel — owner=%d isDialog=%d "
        "inputDisabled=%d name=[%s] (manual control reclaimed)",
        static_cast<int>(g_st.owner), g_st.isDialog ? 1 : 0,
        g_st.inputDisabled ? 1 : 0, g_st.name);
    EndApproach(ApproachOutcome::Cancelled);
    return true;
}

}  // namespace acc::guidance

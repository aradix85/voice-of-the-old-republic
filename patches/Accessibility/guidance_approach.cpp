
#include "guidance_approach_internal.h"

#include "engine_area.h"      // GetObjectPosition / GetObjectName
#include "engine_compass.h"   // EngineYawToCompass / CompassToSector / SectorString
#include "engine_panels.h"    // HasActiveDialogPanel / IsForegroundUiBlocking / SetGlobalDialogState
#include "engine_player.h"    // GetPlayerPosition / SetPlayerInputEnabled
#include "engine_subscreen.h" // WorldIsPaused — a paused world is not a blocked way
#include "guidance_autowalk.h"// CancelMovement
#include "log.h"
#include "speech.h"
#include "strings.h"

namespace acc::guidance {

namespace {

// Outcome of the last approach that ended, and the serial of the latest arm
// (see LastApproachOutcome).
uint32_t        g_armSerial = 0;
ApproachOutcome g_outcome   = ApproachOutcome::None;

}  // namespace

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
    return DistXY(tgt, pos);
}

void MarkWalkStart() {
    Vector pc;
    g_st.haveWalkStart = acc::engine::GetPlayerPosition(pc);
    if (g_st.haveWalkStart) g_st.walkStartPos = pc;
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
    if (DistXYSq(pos, g_st.lastPos) >= kProgressEpsSq) {
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
    // (0,0,0) fallback pos read as "miles away" → bogus "Movement cancelled"
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
    acc::speech::Speak(msg, /*interrupt=*/true);
    acclog::Write("Approach", "movement-cancel — owner=%d isDialog=%d "
        "inputDisabled=%d name=[%s] (manual control reclaimed)",
        static_cast<int>(g_st.owner), g_st.isDialog ? 1 : 0,
        g_st.inputDisabled ? 1 : 0, g_st.name);
    EndApproach(ApproachOutcome::Cancelled);
    return true;
}

}  // namespace acc::guidance

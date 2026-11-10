// Internal to the guidance_approach module (guidance_approach*.cpp); no other file includes
// it. The public interface is guidance_approach.h. Here: what the files share.

#pragma once

#include "guidance_approach.h"

#include <windows.h>
#include <cmath>
#include <cstdio>
#include <cstring>


namespace acc::guidance {

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
extern uint32_t g_refireHandle;
extern DWORD    g_refireAt;
extern ApproachState g_st;

// Helpers shared across the files, by the file that defines them.

// guidance_approach.cpp
void EndApproach(ApproachOutcome outcome);
bool ResolveTargetPos(Vector& out);
float GapTo(const Vector& tgt, const Vector& pos);
void MarkWalkStart();

// guidance_approach_rescue.cpp
bool TryCoordinateWalkRetry(const Vector& tgt);
bool TrySidestep(const Vector& tgt);
void ResumeAfterSidestep();
bool TryReissueUse(float gap);
void OnBlocked(DWORD stalledMs);

}  // namespace acc::guidance

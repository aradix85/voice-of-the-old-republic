// Internal to the guidance_approach module (guidance_approach*.cpp); no other
// file includes it. The public interface is guidance_approach.h.

#pragma once

#include "guidance_approach.h"

#include <windows.h>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace acc::guidance {

constexpr int kSidestepMax = 3;  // sidesteps per arm before giving up

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

    // Sidestep rescue: how many were dispatched, whether one is walking now,
    // where to, and the directions already tried.
    int           sidestepTries = 0;
    bool          stepping      = false;
    Vector        stepPos       = {0.0f, 0.0f, 0.0f};
    float         triedYaw[kSidestepMax] = {};

    // Where the current walk (the first, the coordinate retry or a re-walk)
    // started. "Rooted" means the PC is still within kRootedMaxM of it: a
    // half-metre shuffle on a re-walk must not count as a walk that ran.
    Vector        walkStartPos  = {0.0f, 0.0f, 0.0f};
    bool          haveWalkStart = false;

    // The party hold of a walk that leaves the area: still owed, and since
    // when the leader is standing (0: not holding).
    bool          leavesArea    = false;
    DWORD         holdSince     = 0;
};

extern ApproachState g_st;

// guidance_approach.cpp
void EndApproach(ApproachOutcome outcome);

// The live target position when the object is readable, else the stamped
// fallback; false when neither is available.
bool ResolveTargetPos(Vector& out);

void MarkWalkStart();

// Gives a fresh walk or use a full stall window before the next verdict.
void RestartStallWindow();

// Ends the engine side of an approach: cancels the walk, hands input back
// when the arm took it, and clears the dialog-pending state of a talk.
void ReleaseApproach();

// guidance_approach_gather.cpp. True while it handled the tick.
bool TickPartyHold(const Vector& pos, DWORD now);

// guidance_approach_rescue.cpp
bool TryCoordinateWalkRetry(const Vector& tgt);
bool TrySidestep(const Vector& tgt);
void ResumeAfterSidestep();
bool TryReissueUse(float gap);
void OnBlocked(DWORD stalledMs);

}  // namespace acc::guidance

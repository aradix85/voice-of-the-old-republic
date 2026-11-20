// Internal to the guidance_approach module (guidance_approach*.cpp); no other
// file includes it. The public interface is guidance_approach.h.

#pragma once

#include "guidance_approach.h"

#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace acc::guidance {

constexpr int kSidestepMax = 3;  // sidesteps per arm before giving up
constexpr int kRouteMax    = 128; // hops of one route walk: 640 m, the way
                                  // round the rim of a desert

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
    // The target is someone to talk to or something to use, not a door, a way
    // out, a spot or an enemy: an approach a fight ends (see TickApproach).
    bool          talkOrUse     = false;

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
    // The walk to the floor nearest the target (TryFloorNearestTarget) is
    // one-shot per arm.
    bool          floorTried    = false;

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
    // The step by movement key (TryHandStep) is one-shot per arm.
    bool          handStepped   = false;

    // Route walk (TryRouteWalk): the mod's own path to the target, cut into
    // short hops; the hop being walked, how often it was started again, and
    // the one-shot latch.
    Vector        route[kRouteMax] = {};
    int           routeCount    = 0;
    int           routeAt       = 0;
    int           routeRetries  = 0;
    bool          routing       = false;
    bool          routeTried    = false;

    // Where the current walk (the first, the coordinate retry or a re-walk)
    // started. "Rooted" means the PC is still within kRootedMaxM of it: a
    // half-metre shuffle on a re-walk must not count as a walk that ran.
    Vector        walkStartPos  = {0.0f, 0.0f, 0.0f};
    bool          haveWalkStart = false;

    // The party hold of a walk that leaves the area: still owed, and since
    // when the leader is standing (0: not holding). holdLeftM is the least
    // the companions behind had left to walk, holdCloserAt when they last
    // came closer than that.
    bool          leavesArea    = false;
    DWORD         holdSince     = 0;
    float         holdLeftM     = 0.0f;
    DWORD         holdCloserAt  = 0;
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
// Rooted where the engine starts no walk at all (inside the footprint of a
// container, between people): holds the player's own movement key for a
// moment, toward the freest of forward, back, left and right, as a player
// would step out by hand; the arrival goes on as after a sidestep. True when
// the key is down.
bool TryHandStep(const Vector& tgt);
// Every tick, armed or not: lets that key go when its time is up.
void TickHandStep();
bool TryReissueUse(float gap);
// The PC has settled and the use it was sent to do is still queued: the
// engine cannot get to the use point from this side (a terminal on a desk
// that only the far side comes close to). Walks to the walkable floor nearest
// the target, wherever around it that is; the use is re-fired on arrival.
// True when that walk is in flight.
bool TryFloorNearestTarget(const Vector& tgt);
// True when that walk and its re-fired use still left the use queued: the
// use point cannot be reached, and saying so beats a silent disarm.
bool UseNeverLanded();
// The mod walks its own path over the nav graph, hop by hop of a few metres;
// the arrival acts as after the coordinate retry. For a target the engine
// starts no walk to, though short walks go (it plots no path of its own
// there: the Ebon Hawk's corridors, Davik's hangar after the fight), and for
// one behind a turn-back zone. `why` goes to the log. True when the first hop
// is in flight.
bool TryRouteWalk(const Vector& tgt, const char* why);
// Turn-back zones (turnback_zones.h).
// A conversation ended the approach: when nobody speaks it, the target is
// still far and the PC stands on or beside a script trigger, that trigger
// turned the walk back. Its area becomes a zone, and an Enter's target is
// kept to walk to again.
void LearnTurnback();
// The fresh approach would walk straight into a known zone: walks the mod's
// own path round it instead.
void RouteRoundTurnbacks();
// Drops the target kept by LearnTurnback.
void ForgetTurnbackResume();
// With no approach in flight: once the message has closed and the party has
// been put back, repeats the Enter that was turned back.
void TickTurnbackResume();
// The hop was reached: starts the next one. False after the last hop (the
// tracker's own verdicts take the arrival) or when the next will not start.
bool NextRouteHop();
// A hop stalled short of its point: starts it again, twice at most. False
// when the route is given up.
bool RetryRouteHop();
// The last rung before "way blocked": the engine does not walk it, but the
// region graph may know a way. exit_memory then walks region by region,
// opening the doors, and repeats the request on arrival. Every watched walk
// with a target object passes here, whatever its key or kind of target.
// True when that walk started (the tracker is then armed for its first step).
bool TryRegionWalk();
void OnBlocked(DWORD stalledMs);
// A walk arrived: names the companions that are far behind
// (guidance_approach_gather.cpp).
void TellWhoStayedBehind();
// The approach ended at the target without its act: says "A closed Door
// stands by it." when a closed door is on the line from `from` to the target.
bool SpeakClosedDoorBetween(const Vector& from, const Vector& tgt);

}  // namespace acc::guidance

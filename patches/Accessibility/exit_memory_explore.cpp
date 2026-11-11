// Walking for the player: Ctrl+Shift+N, region by region to the nearest
// unexplored region (opening doors on the way), and Ctrl+L to the nearest loot.

#include "exit_memory_internal.h"

#include "cycle_input.h"       // WalkToNarratedTarget
#include "endar_softlock.h"    // Trask may not try his own door
#include "engine_area.h"
#include "engine_panels.h"     // IsForegroundUiBlocking
#include "engine_player.h"     // position, yaw, party, player creature
#include "engine_subscreen.h"  // WorldIsPaused (explore walk waits)
#include "guidance_approach.h"
#include "guidance_autowalk.h"
#include "hotkeys.h"
#include "interact_dispatch.h" // Ctrl+Shift+N tries a locked door like Enter
#include "narrated_target.h"
#include "log.h"
#include "speech.h"
#include "same_name_suffix.h"
#include "strings.h"

namespace acc::exit_memory {

namespace {

constexpr int   kChainMaxHops  = 40;
constexpr DWORD kChainSettleMs = 500;   // after a hop: door swing, region commit
constexpr float kChainMinMoveM = 1.0f;  // a hop that moved less made no progress

void StopChain(const char* why) {
    if (!g_chain.active) return;
    g_chain.active = false;
    acclog::Write("ExitMemory", "explore walk: stop after %d hop(s), goal %d (%s)",
                  g_chain.hops, g_chain.goal, why);
}

// The closed door on the link from the player's region to the region `hopId`
// next to it, or null.
void* ClosedDoorOnHop(const Graph& g, void* area, int hopId) {
    for (const ClusterLink& l : g.links) {
        const bool thisLink = (l.a == g.start && l.b == hopId) ||
                              (l.b == g.start && l.a == hopId);
        if (thisLink && l.viaDoor) return ClosedDoorNear(area, l.doorPos);
    }
    return nullptr;
}

// One hop of the explore walk: from the player's region to the next region on
// the open way to the goal — through the closed door on that link (walked to
// and opened like Shift+- on it) or to the next region's centre. False when
// there is no open way or no walk started.
bool ChainStep() {
    Vector player;
    if (!acc::engine::GetPlayerPosition(player)) return false;
    void* area = acc::engine::GetCurrentArea();
    Graph g;
    if (!BuildGraph(area, player, g)) return false;
    const int t = g.IndexOf(g_chain.goal);
    if (t < 0 || g.si < 0 || t == g.si) return false;
    if (g.distOpen[t] >= kFar || g.hopOpen[t] < 0) return false;
    const int hop   = g.hopOpen[t];
    const int hopId = g.nodes[hop].id;
    void* door = ClosedDoorOnHop(g, area, hopId);
    uint32_t before = 0;
    acc::guidance::LastApproachOutcome(&before);
    if (door) {
        acc::narrated_target::Stamp(door, acc::engine::GetObjectHandle(door));
        acc::cycle_input::WalkToNarratedTarget();
    } else {
        if (!acc::guidance::WalkTo(g.nodes[hop].centre)) return false;
        acc::guidance::ApproachArm arm;
        arm.owner = acc::guidance::ApproachOwner::Cycle;
        std::snprintf(arm.name, sizeof(arm.name), "%s", g_chain.name.c_str());
        arm.targetPos    = g.nodes[hop].centre;
        arm.speakBlocked = true;
        acc::guidance::ArmApproach(arm);
    }
    uint32_t serial = 0;
    acc::guidance::LastApproachOutcome(&serial);
    if (serial == before) return false;  // nothing was dispatched
    g_chain.doorTries = (door && door == g_chain.door) ? g_chain.doorTries + 1 : 0;
    g_chain.door     = door;
    g_chain.serial   = serial;
    g_chain.endedAt  = 0;
    g_chain.stepFrom = player;
    acclog::Write("ExitMemory", "explore walk: hop %d region %d -> %d%s (goal %d)",
                  g_chain.hops, g.start, hopId, door ? " through a door" : "",
                  g_chain.goal);
    return true;
}

void WalkTowardUnexplored() {
    using acc::strings::Id;
    Overview o;
    if (!CollectOverview(o)) return;
    if (!o.haveTarget || o.targetId < 0) {
        StopChain("nothing left");
        const uint32_t handle =
            o.tryDoor ? acc::engine::GetObjectHandle(o.tryDoor) : 0;
        if (handle != 0) {
            // Only a locked door leads on: try it, exactly as Enter on it.
            // The Trask guard speaks its own reason and refuses.
            if (acc::endar::BlockCompanionAttempt(o.tryDoor)) return;
            acc::narrated_target::Stamp(o.tryDoor, handle);
            acc::interact::DispatchInteract(o.tryDoor, handle,
                                            /*forceRadial=*/false);
            // Queued behind the interact line, which names the door.
            acc::speech::Speak(acc::strings::Get(Id::ExploreTryingDoor),
                         /*interrupt=*/false);
            acclog::Write("ExitMemory", "explore walk: nothing reachable left, "
                          "trying locked door 0x%08x", handle);
            return;
        }
        const uint32_t exitHandle =
            o.exitDoor ? acc::engine::GetObjectHandle(o.exitDoor) : 0;
        char exitName[192] = {0};
        if (exitHandle != 0 &&
            acc::narration::GetSpokenName(o.exitDoor, CycleCategory::Door,
                                          exitName, sizeof(exitName)) &&
            exitName[0]) {
            // Only a way out is left: make it the target and say so. No
            // walk: going through may load another area, so that stays
            // the player's own Enter.
            acc::narrated_target::Stamp(o.exitDoor, exitHandle);
            char line[640];
            std::snprintf(line, sizeof(line),
                          acc::strings::Get(Id::FmtExploreWayOut),
                          Located(exitName, o.exitDoorPos).c_str());
            acc::speech::Speak(line, /*interrupt=*/true);
            acclog::Write("ExitMemory", "explore walk: way out 0x%08x -> [%s]",
                          exitHandle, line);
            return;
        }
        // Nothing reachable left: say why (all explored / behind locks).
        const std::string text = OverviewText(o);
        acc::speech::Speak(text.c_str(), /*interrupt=*/true);
        return;
    }
    g_chain        = ExploreChain{};
    g_chain.active = true;
    g_chain.goal   = o.targetId;
    g_chain.name   = o.targetWhat.empty() ? std::string(acc::strings::Get(Id::ExitAnnoNew))
                                          : o.targetWhat;

    // One line up front: what the walk heads for and how far it is to walk.
    std::string subject = g_chain.name;
    Vector player;
    Graph g;
    if (acc::engine::GetPlayerPosition(player) &&
        BuildGraph(acc::engine::GetCurrentArea(), player, g)) {
        const int t = g.IndexOf(o.targetId);
        if (t >= 0) {
            const std::string way = RouteText(RouteToIndex(g, t, g.nodes[t].centre));
            if (!way.empty()) subject += ", " + way;
        }
    }
    if (!ChainStep()) {
        g_chain.active = false;
        char fail[256];
        std::snprintf(fail, sizeof(fail), acc::strings::Get(Id::FmtGuidingFailed),
                      g_chain.name.c_str());
        acc::speech::Speak(fail, /*interrupt=*/true);
        return;
    }
    char buf[640];
    std::snprintf(buf, sizeof(buf), acc::strings::Get(Id::FmtWalkingTo),
                  subject.c_str());
    acc::speech::Speak(buf, /*interrupt=*/true);
    acclog::Write("ExitMemory", "walk unexplored: %s (goal region %d)", buf,
                  o.targetId);
}

void WalkToNearestLoot() {
    Overview o;
    if (!CollectOverview(o)) return;
    if (o.loot.empty()) {
        acc::speech::Speak(acc::strings::Get(acc::strings::Id::OverviewNothingToSearch),
                     /*interrupt=*/true);
        return;
    }
    const LootSpot& s = o.loot.front();
    if (!s.obj || !s.handle) return;
    acc::narrated_target::Stamp(s.obj, s.handle);
    acc::cycle_input::WalkToNarratedTarget();
    acclog::Write("ExitMemory", "walk loot: %s", s.name.c_str());
}

}  // namespace

void TickExploreChain() {
    if (!g_chain.active) return;
    if (acc::guidance::IsAnyModApproachInFlight()) {
        g_chain.endedAt = 0;
        return;
    }
    uint32_t serial = 0;
    const acc::guidance::ApproachOutcome out =
        acc::guidance::LastApproachOutcome(&serial);
    if (serial != g_chain.serial) return StopChain("another walk took over");
    if (out == acc::guidance::ApproachOutcome::Blocked) return StopChain("blocked");
    if (out == acc::guidance::ApproachOutcome::Cancelled) return StopChain("cancelled");
    if (acc::engine::GetCurrentArea() != g_area) return StopChain("area changed");
    if (g_chain.goal == g_current || g_visited.count(g_chain.goal)) {
        return StopChain("arrived");
    }
    if (acc::engine::IsAnyPartyMemberInCombat()) return StopChain("combat");
    if (acc::engine::HasActiveDialogPanel() ||
        acc::engine::IsForegroundUiBlocking()) {
        return StopChain("panel open");
    }
    const DWORD now = GetTickCount();
    if (g_chain.endedAt == 0) {
        g_chain.endedAt = now;
        return;
    }
    if (now - g_chain.endedAt < kChainSettleMs) return;
    if (acc::engine::WorldIsPaused()) return;  // carries on after unpause
    Vector p;
    if (!acc::engine::GetPlayerPosition(p)) return;
    // A door hop may open the door without a step (the player stood at it);
    // any other hop that left the player where it began made no progress.
    if (!g_chain.door && DistXY(p, g_chain.stepFrom) < kChainMinMoveM) {
        return StopChain("no progress");
    }
    if (g_chain.doorTries >= 2) return StopChain("door did not open");
    if (++g_chain.hops > kChainMaxHops) return StopChain("hop limit");
    if (!ChainStep()) {
        ClusterInfo goal;
        if (acc::room_topology::GetClusterInfo(g_area, g_chain.goal, goal)) {
            acc::guidance::SpeakWayBlocked(g_chain.name.c_str(), goal.centre);
        }
        StopChain("no way on");
    }
}

void PollWalkHotkeys() {
    const bool unexplored =
        acc::hotkeys::Pressed(acc::hotkeys::Action::ExploreWalkUnexplored);
    const bool loot = acc::hotkeys::Pressed(acc::hotkeys::Action::ExploreWalkLoot);
    if (!unexplored && !loot) return;
    Vector unused;
    if (!acc::engine::GetPlayerPosition(unused)) return;
    if (acc::engine::IsForegroundUiBlocking()) return;
    if (unexplored) {
        WalkTowardUnexplored();
    } else {
        WalkToNearestLoot();
    }
}

}  // namespace acc::exit_memory

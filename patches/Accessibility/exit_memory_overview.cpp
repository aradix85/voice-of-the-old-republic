// The exploration overview (Ctrl+N, the bridge's /overview), loot still to
// search, "No way there now", and the loot reminder on leaving a region.

#include "exit_memory_internal.h"

#include "discovery.h"         // loot heard in the overview joins the , / . cycle
#include "engine_area.h"
#include "engine_panels.h"     // IsForegroundUiBlocking
#include "engine_player.h"     // position, yaw, party, player creature
#include "hotkeys.h"
#include "log.h"
#include "menus_modsettings.h"   // ExtendedCycling: list loot map-wide
#include "speech.h"
#include "same_name_suffix.h"
#include "strings.h"

namespace acc::exit_memory {

namespace {

// Overview: loot spots named (the rest are counted).
constexpr int   kLootSpoken  = 5;

// Loot reminder (RemindLootOnLeaving): spoken this long after the walk line,
// queued behind it; not again for the same door within the repeat window;
// at most this many things named.
constexpr DWORD kReminderDelayMs  = 1200;
constexpr DWORD kReminderRepeatMs = 20000;
constexpr int   kReminderNamed    = 3;

// "No way there now" after an Enter on something only behind locked doors:
// queued behind the walk line like the reminder.
constexpr DWORD kNoWayDelayMs = 700;
bool        g_noWayPending    = false;
DWORD       g_noWayAtMs       = 0;

bool        g_reminderPending = false;
DWORD       g_reminderAtMs    = 0;
std::string g_reminderText;
uint32_t    g_reminderDoor    = 0;
DWORD       g_reminderDoorMs  = 0;

// A way-out door the graph cannot route to still counts when it is this
// close in a straight line.
constexpr float kExitDoorNearM = 40.0f;

// Loot left: containers holding items, and items on the ground, nearest
// first (straight line). Every such object in the area, whether or not the
// player has heard it yet: the , / . cycles' discovery tier is the wrong
// filter here (a footlocker not yet walked past is exactly what "left to
// search" is for, and ground items are never in that tier at all). Callers
// decide what of it to speak; `known` says whether the cycles would list it.
void CollectLoot(std::vector<LootSpot>& out) {
    out.clear();
    Vector player;
    if (!acc::engine::GetPlayerPosition(player)) return;
    void* area = acc::engine::GetCurrentArea();
    if (!area) return;
    const bool mapWide = acc::menus::modsettings::GetToggle(
        acc::menus::modsettings::Option::ExtendedCycling);
    acc::engine::AreaObjectIterator it(area);
    while (void* obj = it.Next()) {
        CycleCategory cat;
        if (acc::filter::ObjectMatches(obj, CycleCategory::Container)) {
            cat = CycleCategory::Container;
            if (acc::engine::GetContainerItemCount(obj) <= 0) continue;
        } else if (acc::filter::ObjectMatches(obj, CycleCategory::Item)) {
            cat = CycleCategory::Item;
        } else {
            continue;
        }
        Vector pos;
        if (!acc::engine::GetObjectPosition(obj, pos)) continue;
        char name[160] = {0};
        if (!acc::narration::GetSpokenName(obj, cat, name, sizeof(name)) ||
            !name[0]) {
            continue;
        }
        LootSpot s;
        s.obj      = obj;
        s.handle   = acc::engine::GetObjectHandle(obj);
        s.name     = name;
        s.pos      = pos;
        s.distance = DistXY(pos, player);
        s.walk     = s.distance;
        s.known    = mapWide || acc::discovery::IsDiscovered(obj);
        out.push_back(std::move(s));
    }
    std::stable_sort(out.begin(), out.end(),
                     [](const LootSpot& a, const LootSpot& b) {
                         return a.distance < b.distance;
                     });
}

}  // namespace

void FlushReminder() {
    if (g_noWayPending &&
        static_cast<LONG>(GetTickCount() - g_noWayAtMs) >= 0) {
        g_noWayPending = false;
        Vector unused;
        if (acc::engine::GetPlayerPosition(unused) &&
            !acc::engine::IsForegroundUiBlocking()) {
            const char* line = acc::strings::Get(acc::strings::Id::RouteNoWayLocked);
            acc::speech::Speak(line, /*interrupt=*/false);
            acclog::Write("ExitMemory", "no way: %s", line);
        }
    }
    if (!g_reminderPending) return;
    if (static_cast<LONG>(GetTickCount() - g_reminderAtMs) < 0) return;
    g_reminderPending = false;
    Vector unused;
    if (!acc::engine::GetPlayerPosition(unused)) return;
    // A conversation or a panel took over meanwhile: the moment has passed.
    if (acc::engine::IsForegroundUiBlocking()) return;
    acc::speech::Speak(g_reminderText.c_str(), /*interrupt=*/false);
    acclog::Write("ExitMemory", "loot reminder: %s", g_reminderText.c_str());
}

bool CollectOverview(Overview& o) {
    o = Overview{};
    Vector player;
    if (!acc::engine::GetPlayerPosition(player)) return false;
    void* area = acc::engine::GetCurrentArea();
    if (!area) return false;

    Graph g;
    if (BuildGraph(area, player, g)) {
        o.haveGraph = true;

        int target = -1;
        for (size_t i = 0; i < g.nodes.size(); ++i) {
            // A doorway sliver of an area door is not a place to explore
            // (the region in front of the door stands for it), and neither
            // is the unlisted region the player stands in.
            if (TransitionSliverOwner(area, g.nodes[i].id, nullptr) !=
                    g.nodes[i].id ||
                (g.startUnlisted && static_cast<int>(i) == g.si)) {
                continue;
            }
            const bool visited = g_visited.count(g.nodes[i].id) != 0;
            const bool reachable = g.distAll[i] < kFar;
            if (visited || reachable) ++o.total;
            if (visited) ++o.explored;
            if (visited || !reachable || static_cast<int>(i) == g.si) continue;
            if (g.distOpen[i] >= kFar) {
                ++o.lockedAway;
            } else if (target < 0 || g.distOpen[i] < g.distOpen[target]) {
                target = static_cast<int>(i);
            }
        }

        if (target >= 0 && g.hopOpen[target] >= 0) {
            o.haveTarget = true;
            o.targetId   = g.nodes[target].id;
            const auto landmarks = LandmarkRegions(area);
            o.targetWhat = Destination(area, g.nodes[target].id, landmarks,
                                       std::string());
            const int hop = g.hopOpen[target];
            const int hopId = g.nodes[hop].id;
            o.stepPos = g.nodes[hop].centre;
            // The closed door on the link into the first region, if any:
            // walking there means opening it.
            for (const ClusterLink& l : g.links) {
                const bool thisLink = (l.a == g.start && l.b == hopId) ||
                                      (l.b == g.start && l.a == hopId);
                if (!thisLink || !l.viaDoor) continue;
                o.stepDoor = ClosedDoorNear(area, l.doorPos);
                break;
            }
            // The exit of the current region that leads to the first region
            // on the way.
            Vector viaPos{};
            o.via = ExitToward(g, hop, &viaPos);
            o.targetPos = o.via.empty() ? g.nodes[hop].centre : viaPos;
        }

        // Nothing left to walk to, but unexplored regions behind locked
        // doors: the nearest locked door with a reachable region on one side
        // and an unexplored one on the other. Some open on the attempt (Door
        // 1 on the Endar Spire bridge), so a count alone is not enough.
        if (target < 0 && o.lockedAway > 0) {
            float best = kFar;
            for (const ClusterLink& l : g.links) {
                if (!l.viaDoor) continue;
                const int a = g.IndexOf(l.a);
                const int b = g.IndexOf(l.b);
                if (a < 0 || b < 0) continue;
                int nearI = -1;
                int farI  = -1;
                if (g.distOpen[a] < kFar && g.distOpen[b] >= kFar) {
                    nearI = a;
                    farI  = b;
                } else if (g.distOpen[b] < kFar && g.distOpen[a] >= kFar) {
                    nearI = b;
                    farI  = a;
                } else {
                    continue;
                }
                if (g_visited.count(g.nodes[farI].id)) continue;
                void* door = ClosedDoorNear(area, l.doorPos);
                if (!door || !acc::engine::IsDoorLocked(door) ||
                    acc::engine::IsDoorCosmetic(door)) {
                    continue;
                }
                Vector dp = l.doorPos;
                acc::engine::GetObjectPosition(door, dp);
                const float walk =
                    (nearI == g.si)
                        ? DistXY(player, dp)
                        : g.distOpen[nearI] + DistXY(g.nodes[nearI].centre, dp);
                if (walk < best) {
                    best          = walk;
                    o.tryDoor     = door;
                    o.tryDoorPos  = dp;
                }
            }
        }

        // Still nothing: the nearest door the graph joins nothing through,
        // usually the way out of the area (the Taris hideout is one region,
        // and the far side of its exit door is folded into it; the door
        // carries no transition text).
        if (target < 0 && !o.tryDoor) {
            float best = kFar;
            acc::engine::ForEachDoor(area, [&](void* door, const Vector& dp) {
                if (acc::engine::IsDoorCosmetic(door)) return;
                int nearId = kClusterIdNone;
                int farId  = kClusterIdNone;
                if (DoorSides(g, dp, nearId, farId)) return;
                const float d = DistXY(player, dp);
                const Route r = RouteTo(g, dp);
                const bool reachable =
                    r.kind == Route::Here || r.kind == Route::Open ||
                    (r.kind == Route::Unknown && d <= kExitDoorNearM);
                if (!reachable) return;
                const float walk =
                    r.kind == Route::Open ? static_cast<float>(r.walkM) : d;
                if (walk < best) {
                    best          = walk;
                    o.exitDoor    = door;
                    o.exitDoorPos = dp;
                }
            });
        }
    }

    // Loot left, with the way to each spot in another region. Reachable
    // spots first, nearest walk first; what only lies behind locked doors
    // (or that the graph has no link to) goes last.
    CollectLoot(o.loot);
    if (o.haveGraph) {
        for (LootSpot& s : o.loot) {
            const Route r = RouteTo(g, s.pos);
            s.way = RouteText(r);
            if (r.kind == Route::Open) s.walk = static_cast<float>(r.walkM);
            s.reachable = r.kind != Route::Locked && r.kind != Route::Unreachable;
            // Not heard yet and not reachable now (behind a locked door, or
            // somewhere the graph has no way to): leave it out, it would
            // only name what lies ahead in the story.
            if (!s.known) {
                const bool nearUnknown =
                    r.kind == Route::Unknown && s.distance <= kHereRadiusM;
                s.keep = r.kind == Route::Here || r.kind == Route::Open ||
                         nearUnknown;
            }
        }
    } else {
        for (LootSpot& s : o.loot) s.keep = s.known || s.distance <= kHereRadiusM;
    }
    o.loot.erase(std::remove_if(o.loot.begin(), o.loot.end(),
                                [](const LootSpot& s) { return !s.keep; }),
                 o.loot.end());
    if (o.haveGraph) {
        std::stable_sort(o.loot.begin(), o.loot.end(),
                         [](const LootSpot& a, const LootSpot& b) {
                             if (a.reachable != b.reachable) return a.reachable;
                             return a.walk < b.walk;
                         });
    }
    return true;
}

std::string OverviewText(const Overview& o) {
    using acc::strings::Id;
    std::vector<std::string> parts;
    char buf[768];
    if (!o.haveGraph) {
        parts.push_back(acc::strings::Get(Id::OverviewNoMap));
    } else {
        std::snprintf(buf, sizeof(buf), acc::strings::Get(Id::FmtOverviewExplored),
                      o.explored, o.total);
        parts.push_back(buf);
        if (o.haveTarget) {
            std::string subject = o.via;
            if (!o.targetWhat.empty()) {
                if (!subject.empty()) subject += ", ";
                subject += o.targetWhat;
            }
            if (subject.empty()) subject = acc::strings::Get(Id::ExitAnnoNew);
            std::snprintf(buf, sizeof(buf), acc::strings::Get(Id::FmtOverviewNearest),
                          Located(subject, o.targetPos).c_str());
            parts.push_back(buf);
        } else if (o.lockedAway == 0) {
            parts.push_back(acc::strings::Get(Id::OverviewAllExplored));
        }
        if (o.lockedAway > 0) {
            std::snprintf(buf, sizeof(buf),
                          acc::strings::Get(Id::FmtOverviewLockedAway), o.lockedAway);
            parts.push_back(buf);
        }
        if (!o.haveTarget && o.tryDoor) {
            char name[192] = {0};
            if (acc::narration::GetSpokenName(o.tryDoor, CycleCategory::Door,
                                              name, sizeof(name)) &&
                name[0]) {
                std::snprintf(buf, sizeof(buf),
                              acc::strings::Get(Id::FmtOverviewTryDoor),
                              Located(name, o.tryDoorPos).c_str());
                parts.push_back(buf);
            }
        } else if (!o.haveTarget && o.exitDoor) {
            char name[192] = {0};
            if (acc::narration::GetSpokenName(o.exitDoor, CycleCategory::Door,
                                              name, sizeof(name)) &&
                name[0]) {
                std::snprintf(buf, sizeof(buf),
                              acc::strings::Get(Id::FmtOverviewWayOut),
                              Located(name, o.exitDoorPos).c_str());
                parts.push_back(buf);
            }
        }
    }
    if (o.loot.empty()) {
        parts.push_back(acc::strings::Get(Id::OverviewNothingToSearch));
    } else {
        std::snprintf(buf, sizeof(buf), acc::strings::Get(Id::FmtOverviewToSearch),
                      static_cast<int>(o.loot.size()));
        parts.push_back(buf);
        std::string spots;
        int n = 0;
        for (const LootSpot& s : o.loot) {
            if (n == kLootSpoken) break;
            if (!spots.empty()) spots += "; ";
            spots += Located(s.name, s.pos);
            if (!s.way.empty()) spots += ", " + s.way;
            ++n;
        }
        parts.push_back(spots);
    }
    std::string text;
    for (const std::string& p : parts) {
        if (p.empty()) continue;
        if (!text.empty()) text += ". ";
        text += p;
    }
    if (!text.empty() && text.back() != '.') text += '.';
    return text;
}

void PollOverviewHotkey() {
    if (!acc::hotkeys::Pressed(acc::hotkeys::Action::ExploreOverview)) return;
    Vector unused;
    if (!acc::engine::GetPlayerPosition(unused)) return;
    if (acc::engine::IsForegroundUiBlocking()) return;
    Overview o;
    if (!CollectOverview(o)) return;
    // The loot just named is now known: let the , / . cycles find it too.
    int named = 0;
    for (const LootSpot& s : o.loot) {
        if (named++ == kLootSpoken) break;
        if (s.obj) acc::discovery::Record(s.obj);
    }
    // Where am I first, then the area as a whole.
    std::string text = CurrentRegionText();
    if (!text.empty() && text.back() != '.') text += '.';
    if (!text.empty()) text += ' ';
    text += OverviewText(o);
    acc::speech::Speak(text.c_str(), /*interrupt=*/true);
    acclog::Write("ExitMemory", "overview: %s", text.c_str());
}

void WarnIfBehindLock(void* target) {
    if (!target) return;
    Vector pos;
    if (!acc::engine::GetObjectPosition(target, pos)) return;
    bool behind = false;
    void* objs[1] = {target};
    if (!UnreachableFlags(objs, &pos, 1, &behind) || !behind) return;
    // Only behind a lock, not merely without a known link: that much the
    // graph can tell for sure (an unlinked region may still be reached by a
    // lift or a scene).
    Vector player;
    if (!acc::engine::GetPlayerPosition(player)) return;
    Graph g;
    if (!BuildGraph(acc::engine::GetCurrentArea(), player, g)) return;
    int t = -1;
    if (acc::filter::ObjectMatches(target, CycleCategory::Door)) {
        int nearId = kClusterIdNone;
        int farId  = kClusterIdNone;
        if (DoorSides(g, pos, nearId, farId)) t = g.IndexOf(nearId);
    }
    if (t < 0) t = g.IndexOf(RegionAt(g.area, pos));
    if (t < 0 || g.distAll[t] >= kFar) return;
    g_noWayPending = true;
    g_noWayAtMs    = GetTickCount() + kNoWayDelayMs;
    acclog::Write("ExitMemory", "no way queued: target 0x%08x behind a lock",
                  acc::engine::GetObjectHandle(target));
}

void RemindLootOnLeaving(void* target) {
    using acc::strings::Id;
    if (!target) return;
    // Not leaving through a door that is locked shut.
    if (acc::filter::ObjectMatches(target, CycleCategory::Door) &&
        acc::engine::IsDoorLocked(target) && !acc::engine::IsDoorOpen(target)) {
        return;
    }
    const uint32_t door = acc::engine::GetObjectHandle(target);
    const DWORD now = GetTickCount();
    if (door != 0 && door == g_reminderDoor &&
        now - g_reminderDoorMs < kReminderRepeatMs) {
        return;
    }
    Vector player;
    if (!acc::engine::GetPlayerPosition(player)) return;
    void* area = acc::engine::GetCurrentArea();
    if (!area) return;
    const int here = (area == g_area && g_current >= 0) ? g_current
                                                        : RegionAt(area, player);
    if (here < 0) return;

    std::vector<LootSpot> loot;
    CollectLoot(loot);
    std::string list;
    int named = 0;
    int total = 0;
    for (const LootSpot& s : loot) {
        if (RegionAt(area, s.pos) != here) continue;
        ++total;
        if (named == kReminderNamed) continue;
        if (!list.empty()) list += "; ";
        list += s.name;
        ++named;
    }
    if (total == 0) return;
    if (total > named) {
        char more[64];
        std::snprintf(more, sizeof(more),
                      acc::strings::Get(Id::FmtRegionHereMore), total - named);
        list += "; ";
        list += more;
    }
    char buf[640];
    std::snprintf(buf, sizeof(buf), acc::strings::Get(Id::FmtLootReminder),
                  list.c_str());
    g_reminderText    = buf;
    g_reminderPending = true;
    g_reminderAtMs    = now + kReminderDelayMs;
    g_reminderDoor    = door;
    g_reminderDoorMs  = now;
    acclog::Write("ExitMemory", "loot reminder queued (region %d, door 0x%08x): %s",
                  here, door, buf);
}

}  // namespace acc::exit_memory

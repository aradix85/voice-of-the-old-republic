// Local bridge: what is in the area, for /nearby, /overview, /objects and
// /rooms.

#include <algorithm>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

#include "bridge_internal.h"
#include "cycle_state.h"
#include "engine_area.h"
#include "examine_view.h"
#include "exit_memory.h"
#include "same_name_suffix.h"

namespace acc::bridge {

namespace {

using C = acc::filter::CycleCategory;

constexpr size_t kMaxObjects = 256;

const char* Bool(bool b) { return b ? "true" : "false"; }

// "category", "name" (as the mod speaks it), "handle", "x" and "y" of one
// object, without braces.
std::string ObjectFields(C cat, void* obj, const Vector& p) {
    char nbuf[256] = {0};
    std::string name = "?";
    if (acc::narration::GetSpokenName(obj, cat, nbuf, sizeof(nbuf)) && nbuf[0])
        name = ToUtf8(nbuf);
    return "\"category\":" + Json(CategoryKey(cat)) + ",\"name\":" + Json(name) +
           ",\"handle\":" + HandleJson(acc::engine::GetObjectHandle(obj)) +
           ",\"x\":" + Num(p.x) + ",\"y\":" + Num(p.y);
}

// The live state /objects adds per category.
std::string StateFields(C cat, void* obj) {
    switch (cat) {
        case C::Door:
            return std::string(",\"open\":") + Bool(acc::engine::IsDoorOpen(obj)) +
                   ",\"locked\":" + Bool(acc::engine::IsDoorLocked(obj)) +
                   ",\"cosmetic\":" + Bool(acc::engine::IsDoorCosmetic(obj));
        case C::Container:
            return ",\"items\":" + std::to_string(acc::engine::GetContainerItemCount(obj));
        case C::Npc:
            return std::string(",\"dead\":") + Bool(acc::examine_view::IsCreatureDead(obj)) +
                   ",\"hostile\":" + Bool(acc::examine_view::IsHostileCreature(obj));
        default:
            return "";
    }
}

}  // namespace

std::string BuildNearby(const Params& params) {
    PlayerFrame f = ReadPlayerFrame();
    if (!f.ok) return kNotInWorld;
    const float radius = static_cast<float>(std::atof(Param(params, "radius").c_str()));

    std::string s = "{\"radius\":" + Num(radius) + ",\"objects\":[";
    bool first = true;
    for (int c = 0; c < static_cast<int>(C::Count_); ++c) {
        const auto cat = static_cast<C>(c);
        acc::cycle::CategoryListing listing;
        if (!acc::cycle::BuildCategoryListing(cat, listing)) continue;
        for (int i = 0; i < listing.count; ++i) {
            if (listing.isPin[i] || !listing.objs[i]) continue;
            const Vector& p = listing.positions[i];
            if (radius > 0 && DistXY(f.pos, p) > radius) continue;
            if (!first) s += ",";
            first = false;
            s += "{" + ObjectFields(cat, listing.objs[i], p) +
                 ",\"where\":" + RelativeJson(f, p) + "}";
        }
    }
    return s + "]}";
}

std::string BuildOverview() {
    acc::exit_memory::Overview o;
    if (!acc::exit_memory::CollectOverview(o)) return kNotInWorld;
    PlayerFrame f = ReadPlayerFrame();
    std::string s = "{\"have_map\":";
    s += Bool(o.haveGraph);
    const std::string here = acc::exit_memory::CurrentRegionText();
    if (!here.empty()) s += ",\"here\":" + JsonGame(here.c_str());
    s += ",\"explored\":" + std::to_string(o.explored) +
         ",\"total\":" + std::to_string(o.total) +
         ",\"locked_away\":" + std::to_string(o.lockedAway);
    if (o.haveTarget) {
        s += ",\"nearest_unexplored\":{\"what\":" + JsonGame(o.targetWhat.c_str()) +
             ",\"via\":" + JsonGame(o.via.c_str()) +
             ",\"x\":" + Num(o.targetPos.x) + ",\"y\":" + Num(o.targetPos.y) +
             ",\"where\":" + RelativeJson(f, o.targetPos) + "}";
    }
    s += ",\"to_search\":[";
    for (size_t i = 0; i < o.loot.size(); ++i) {
        const auto& l = o.loot[i];
        if (i) s += ",";
        s += "{\"name\":" + JsonGame(l.name.c_str()) +
             ",\"handle\":" + HandleJson(l.handle) +
             ",\"x\":" + Num(l.pos.x) + ",\"y\":" + Num(l.pos.y) +
             ",\"where\":" + RelativeJson(f, l.pos) +
             ",\"reachable\":" + Bool(l.reachable);
        if (!l.way.empty()) s += ",\"way\":" + JsonGame(l.way.c_str());
        s += "}";
    }
    s += "],\"text\":" + JsonGame(acc::exit_memory::OverviewText(o).c_str()) + "}";
    return s;
}

// Every door, loot container, ground item and creature in the area with its
// live state and tag, heard before or not, for the map tool (kotor_kaart.py
// --live) to lay over the static game files: which doors are open, which
// containers are empty, who is where now, what can be reached, and what the
// game added (remains of the fallen).
std::string BuildObjects() {
    PlayerFrame f = ReadPlayerFrame();
    void* area = acc::engine::GetCurrentArea();
    if (!f.ok || !area) return kNotInWorld;

    std::vector<void*> objs;
    std::vector<Vector> poss;
    std::vector<C> cats;
    acc::engine::AreaObjectIterator it(area);
    while (void* obj = it.Next()) {
        C cat;
        if (acc::filter::ObjectMatches(obj, C::Door)) cat = C::Door;
        else if (acc::filter::ObjectMatches(obj, C::Container)) cat = C::Container;
        else if (acc::filter::ObjectMatches(obj, C::Item)) cat = C::Item;
        else if (acc::filter::ObjectMatches(obj, C::Npc)) cat = C::Npc;
        else continue;
        Vector p;
        if (!acc::engine::GetObjectPosition(obj, p)) continue;
        objs.push_back(obj);
        poss.push_back(p);
        cats.push_back(cat);
        if (objs.size() >= kMaxObjects) break;
    }
    bool behind[kMaxObjects] = {};
    if (!objs.empty() &&
        !acc::exit_memory::UnreachableFlags(objs.data(), poss.data(),
                                            static_cast<int>(objs.size()), behind)) {
        std::fill(behind, behind + kMaxObjects, false);
    }

    std::string s = "{\"objects\":[";
    for (size_t i = 0; i < objs.size(); ++i) {
        char tag[64] = {0};
        acc::engine::GetObjectTag(objs[i], tag, sizeof(tag));
        if (i) s += ",";
        s += "{" + ObjectFields(cats[i], objs[i], poss[i]) +
             ",\"tag\":" + JsonGame(tag) +
             ",\"reachable\":" + Bool(!behind[i]) + StateFields(cats[i], objs[i]) +
             ",\"where\":" + RelativeJson(f, poss[i]) + "}";
    }
    return s + "]}";
}

// The area's regions (the mod's room graph): kind, visited, reachable now,
// and which regions touch. For the map tool's room summary.
std::string BuildRooms() {
    PlayerFrame f = ReadPlayerFrame();
    std::vector<acc::exit_memory::RegionSummary> rooms;
    std::vector<std::pair<int, int>> links;
    if (!acc::exit_memory::ListRegions(rooms, links))
        return "{\"error\":\"no room map for this area (yet)\"}";
    std::string s = "{\"rooms\":[";
    for (size_t i = 0; i < rooms.size(); ++i) {
        const auto& r = rooms[i];
        if (i) s += ",";
        s += "{\"id\":" + std::to_string(r.id) + ",\"kind\":" + Json(r.kind) +
             ",\"x\":" + Num(r.centre.x) + ",\"y\":" + Num(r.centre.y) +
             ",\"visited\":" + Bool(r.visited) +
             ",\"here\":" + Bool(r.here) +
             ",\"reachable\":" + Bool(r.reachable) +
             ",\"behind_lock\":" + Bool(r.behindLock) +
             ",\"walk\":" + Num(r.walk) +
             ",\"where\":" + RelativeJson(f, r.centre) + "}";
    }
    s += "],\"links\":[";
    for (size_t i = 0; i < links.size(); ++i) {
        if (i) s += ",";
        s += "[" + std::to_string(links[i].first) + "," +
             std::to_string(links[i].second) + "]";
    }
    return s + "]}";
}

}  // namespace acc::bridge

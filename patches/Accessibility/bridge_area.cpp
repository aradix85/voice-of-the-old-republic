// Local bridge: what is in the area, for /nearby, /overview, /objects and /rooms.

#include "bridge_internal.h"

namespace acc::bridge {

std::string BuildNearby(float radius) {
    if (!acc::game::IsKotor1())
        return "{\"error\":\"nearby is KOTOR 1 only for now\"}";
    PlayerFrame f = ReadPlayerFrame();
    if (!f.ok) return "{\"error\":\"no player position (not in the world?)\"}";

    std::string s = "{\"radius\":" + Num(radius) + ",\"objects\":[";
    bool first = true;
    for (int c = 0; c < static_cast<int>(acc::filter::CycleCategory::Count_);
         ++c) {
        auto cat = static_cast<acc::filter::CycleCategory>(c);
        acc::cycle::CategoryListing listing;
        if (!acc::cycle::BuildCategoryListing(cat, listing)) continue;
        for (int i = 0; i < listing.count; ++i) {
            if (listing.isPin[i] || !listing.objs[i]) continue;
            const Vector& p = listing.positions[i];
            float dx = p.x - f.pos.x, dy = p.y - f.pos.y;
            float dist = std::sqrt(dx * dx + dy * dy);
            if (radius > 0 && dist > radius) continue;
            void* obj = listing.objs[i];
            char hbuf[16];
            std::snprintf(hbuf, sizeof(hbuf), "0x%08x",
                          acc::engine::GetObjectHandle(obj));
            char nbuf[256] = {0};
            std::string name = "?";
            if (acc::narration::GetSpokenName(obj, cat, nbuf, sizeof(nbuf)) &&
                nbuf[0])
                name = ToUtf8(nbuf);
            if (!first) s += ",";
            first = false;
            s += "{\"category\":" + Json(CategoryKey(cat)) +
                 ",\"name\":" + Json(name) +
                 ",\"handle\":" + Json(hbuf) +
                 ",\"x\":" + Num(p.x) + ",\"y\":" + Num(p.y) +
                 ",\"where\":" + RelativeJson(f, p) + "}";
        }
    }
    s += "]}";
    return s;
}

std::string BuildOverview() {
    if (!acc::game::IsKotor1())
        return "{\"error\":\"overview is KOTOR 1 only for now\"}";
    acc::exit_memory::Overview o;
    if (!acc::exit_memory::CollectOverview(o))
        return "{\"error\":\"no player position (not in the world?)\"}";
    PlayerFrame f = ReadPlayerFrame();
    std::string s = "{\"have_map\":";
    s += o.haveGraph ? "true" : "false";
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
        char hbuf[16];
        std::snprintf(hbuf, sizeof(hbuf), "0x%08x", l.handle);
        s += "{\"name\":" + JsonGame(l.name.c_str()) +
             ",\"handle\":" + Json(hbuf) +
             ",\"x\":" + Num(l.pos.x) + ",\"y\":" + Num(l.pos.y) +
             ",\"where\":" + RelativeJson(f, l.pos) +
             ",\"reachable\":" + (l.reachable ? "true" : "false");
        if (!l.way.empty()) s += ",\"way\":" + JsonGame(l.way.c_str());
        s += "}";
    }
    s += "],\"text\":" + JsonGame(acc::exit_memory::OverviewText(o).c_str()) + "}";
    return s;
}

// Every door, loot container and ground item in the area with its live state,
// heard before or not, for the map tool (kotor_kaart.py --live) to lay over
// the static game files: which doors are open, which containers are empty,
// what can be reached now, and what the game added (remains of the fallen).
std::string BuildObjects() {
    if (!acc::game::IsKotor1())
        return "{\"error\":\"objects is KOTOR 1 only for now\"}";
    PlayerFrame f = ReadPlayerFrame();
    void* area = acc::engine::GetCurrentArea();
    if (!f.ok || !area) return "{\"error\":\"no player position (not in the world?)\"}";

    struct Entry {
        void* obj;
        acc::filter::CycleCategory cat;
        Vector pos;
    };
    std::vector<Entry> found;
    acc::engine::AreaObjectIterator it(area);
    while (void* obj = it.Next()) {
        using C = acc::filter::CycleCategory;
        C cat;
        if (acc::filter::ObjectMatches(obj, C::Door)) cat = C::Door;
        else if (acc::filter::ObjectMatches(obj, C::Container)) cat = C::Container;
        else if (acc::filter::ObjectMatches(obj, C::Item)) cat = C::Item;
        else continue;
        Vector p;
        if (!acc::engine::GetObjectPosition(obj, p)) continue;
        found.push_back(Entry{obj, cat, p});
        if (found.size() >= 256) break;
    }
    std::vector<void*> objs;
    std::vector<Vector> poss;
    for (const Entry& e : found) {
        objs.push_back(e.obj);
        poss.push_back(e.pos);
    }
    std::vector<char> behind(found.size(), 0);
    if (!found.empty()) {
        bool flags[256] = {};
        if (acc::exit_memory::UnreachableFlags(objs.data(), poss.data(),
                                               static_cast<int>(found.size()),
                                               flags)) {
            for (size_t i = 0; i < found.size(); ++i) behind[i] = flags[i] ? 1 : 0;
        }
    }

    std::string s = "{\"objects\":[";
    for (size_t i = 0; i < found.size(); ++i) {
        const Entry& e = found[i];
        char nbuf[256] = {0};
        std::string name = "?";
        if (acc::narration::GetSpokenName(e.obj, e.cat, nbuf, sizeof(nbuf)) && nbuf[0])
            name = ToUtf8(nbuf);
        char hbuf[16];
        std::snprintf(hbuf, sizeof(hbuf), "0x%08x", acc::engine::GetObjectHandle(e.obj));
        if (i) s += ",";
        s += "{\"category\":" + Json(CategoryKey(e.cat)) +
             ",\"name\":" + Json(name) + ",\"handle\":" + Json(hbuf) +
             ",\"x\":" + Num(e.pos.x) + ",\"y\":" + Num(e.pos.y) +
             ",\"reachable\":" + (behind[i] ? "false" : "true");
        if (e.cat == acc::filter::CycleCategory::Door) {
            s += std::string(",\"open\":") +
                 (acc::engine::IsDoorOpen(e.obj) ? "true" : "false") +
                 ",\"locked\":" + (acc::engine::IsDoorLocked(e.obj) ? "true" : "false") +
                 ",\"cosmetic\":" + (acc::engine::IsDoorCosmetic(e.obj) ? "true" : "false");
        } else if (e.cat == acc::filter::CycleCategory::Container) {
            s += ",\"items\":" + std::to_string(acc::engine::GetContainerItemCount(e.obj));
        }
        s += ",\"where\":" + RelativeJson(f, e.pos) + "}";
    }
    s += "]}";
    return s;
}

// The area's regions (the mod's room graph): kind, visited, reachable now,
// and which regions touch. For the map tool's room summary.
std::string BuildRooms() {
    if (!acc::game::IsKotor1())
        return "{\"error\":\"rooms is KOTOR 1 only for now\"}";
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
             ",\"visited\":" + (r.visited ? "true" : "false") +
             ",\"here\":" + (r.here ? "true" : "false") +
             ",\"reachable\":" + (r.reachable ? "true" : "false") +
             ",\"behind_lock\":" + (r.behindLock ? "true" : "false") +
             ",\"walk\":" + Num(r.walk) +
             ",\"where\":" + RelativeJson(f, r.centre) + "}";
    }
    s += "],\"links\":[";
    for (size_t i = 0; i < links.size(); ++i) {
        if (i) s += ",";
        s += "[" + std::to_string(links[i].first) + "," +
             std::to_string(links[i].second) + "]";
    }
    s += "]}";
    return s;
}

}  // namespace acc::bridge

// Local bridge — see bridge.h for the contract.
//
// Winsock must be included before <windows.h>.
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <bcrypt.h>

#include "bridge.h"

#include <cctype>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "cycle_state.h"
#include "engine_area.h"
#include "engine_compass.h"
#include "engine_game.h"
#include "engine_manager.h"
#include "engine_panels.h"
#include "engine_player.h"
#include "engine_reads.h"
#include "exit_memory.h"
#include "filter_objects.h"
#include "guidance_autowalk.h"
#include "interact_dispatch.h"
#include "key_inject.h"
#include "menus_chain.h"
#include "menus_extract.h"
#include "log.h"
#include "mod_version.h"
#include "narrated_target.h"
#include "party_status.h"
#include "prism.h"
#include "same_name_suffix.h"
#include "strfmt.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "bcrypt.lib")

namespace acc::bridge {

namespace {

constexpr int   kFirstPort        = 47781;
constexpr int   kPortAttempts     = 10;
constexpr DWORD kSnapshotPeriodMs = 250;
constexpr DWORD kJobTimeoutMs     = 2000;
constexpr size_t kSpokenCapacity  = 300;

// ---------------------------------------------------------------------------
// Text helpers

// Game codepage (what the engine and the speech path hand us) -> UTF-8.
std::string ToUtf8(const char* text) {
    if (!text || !*text) return std::string();
    UINT cp = prism::GetSpeechCodepage();
    int wlen = MultiByteToWideChar(cp, 0, text, -1, nullptr, 0);
    if (wlen <= 0) return std::string(text);
    std::wstring w(static_cast<size_t>(wlen), L'\0');
    MultiByteToWideChar(cp, 0, text, -1, &w[0], wlen);
    int ulen = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0,
                                   nullptr, nullptr);
    if (ulen <= 0) return std::string(text);
    std::string u(static_cast<size_t>(ulen), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &u[0], ulen, nullptr,
                        nullptr);
    if (!u.empty() && u.back() == '\0') u.pop_back();
    return u;
}

std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return std::wstring();
    int wlen = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return std::wstring();
    std::wstring w(static_cast<size_t>(wlen), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], wlen);
    if (!w.empty() && w.back() == L'\0') w.pop_back();
    return w;
}

// Escape an UTF-8 string as a JSON string literal (with quotes).
std::string Json(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    out.push_back('"');
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out.push_back(static_cast<char>(c));
                }
        }
    }
    out.push_back('"');
    return out;
}

std::string JsonGame(const char* gameText) { return Json(ToUtf8(gameText)); }

std::string Num(double v, int decimals = 1) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.*f", decimals, v);
    return buf;
}

const char* kDir8[8] = {"north", "northeast", "east", "southeast",
                        "south", "southwest", "west", "northwest"};

// Compass bearing (0 = north, clockwise) of the vector dx,dy in the engine
// frame (+X east, +Y north).
float BearingDeg(float dx, float dy) {
    float engineYaw = std::atan2(dy, dx) * 57.29577951308232f;
    return acc::engine::EngineYawToCompass(engineYaw);
}

const char* Dir8(float compassDeg) {
    return kDir8[acc::engine::CompassToSector(compassDeg)];
}

// ---------------------------------------------------------------------------
// Shared state between the socket thread and the main thread

struct Spoken {
    uint32_t    id;
    DWORD       ms;
    std::string text;  // UTF-8
};

struct Job {
    std::string                        path;
    std::map<std::string, std::string> params;
    std::string                        result;   // JSON body
    bool                               done = false;
};

std::mutex              g_mu;
std::condition_variable g_cv;
std::deque<Spoken>      g_spoken;
uint32_t                g_nextSpokenId = 1;
std::string             g_snapshot = "{\"ready\":false}";
DWORD                   g_snapshotMs = 0;
DWORD                   g_lastTickMs = 0;
std::deque<std::shared_ptr<Job>> g_jobs;

bool        g_started = false;
int         g_port = 0;
std::string g_token;

// ---------------------------------------------------------------------------
// Main-thread readers (all engine access goes through the mod's SEH-guarded
// helpers, so nothing here needs __try).

struct PlayerFrame {
    bool   ok = false;
    Vector pos{0, 0, 0};
    float  yawDeg = 0;      // engine yaw: 0 = east, CCW
    bool   yawOk = false;
};

PlayerFrame ReadPlayerFrame() {
    PlayerFrame f;
    f.ok = acc::engine::GetPlayerPosition(f.pos);
    f.yawOk = acc::engine::GetPlayerYawDegrees(f.yawDeg);
    return f;
}

// Name exactly as the in-world cycle speaks it (ordinal + state words),
// falling back to the plain engine name.
std::string SpokenName(void* obj) {
    char buf[256] = {0};
    for (int c = 0; c < static_cast<int>(acc::filter::CycleCategory::Count_);
         ++c) {
        auto cat = static_cast<acc::filter::CycleCategory>(c);
        if (acc::filter::ObjectMatches(obj, cat) &&
            acc::narration::GetSpokenName(obj, cat, buf, sizeof(buf)) &&
            buf[0]) {
            return ToUtf8(buf);
        }
    }
    buf[0] = 0;
    if (acc::engine::GetObjectName(obj, buf, sizeof(buf)) && buf[0])
        return ToUtf8(buf);
    return "?";
}

std::string RelativeJson(const PlayerFrame& f, const Vector& p) {
    if (!f.ok) return "null";
    float dx = p.x - f.pos.x, dy = p.y - f.pos.y;
    float dist = std::sqrt(dx * dx + dy * dy);
    float bearing = BearingDeg(dx, dy);
    std::string s = "{\"distance\":" + Num(dist) +
                    ",\"bearing\":" + Num(bearing, 0) +
                    ",\"direction\":" + Json(Dir8(bearing));
    if (f.yawOk) {
        int clock = acc::engine::ClockPosition(f.yawDeg, dx, dy);
        s += ",\"clock\":" + std::to_string(clock);
    }
    s += "}";
    return s;
}

std::string CreatureJson(void* serverCreature) {
    std::string name = "?";
    char buf[128] = {0};
    if (acc::engine::GetObjectName(serverCreature, buf, sizeof(buf)) && buf[0])
        name = ToUtf8(buf);
    int hp = acc::engine::ReadCreatureCurrentHitPoints(serverCreature);
    int maxHp = acc::engine::ReadCreatureMaxHitPoints(serverCreature);
    char handle[16];
    std::snprintf(handle, sizeof(handle), "0x%08x",
                  acc::engine::GetObjectHandle(serverCreature));
    std::string s = "{\"name\":" + Json(name) + ",\"hp\":" + std::to_string(hp) +
                    ",\"max_hp\":" + std::to_string(maxHp) +
                    ",\"handle\":" + Json(handle);
    int security = 0;
    if (acc::party_status::SecurityOf(serverCreature, security))
        s += ",\"security\":" + std::to_string(security);
    return s + "}";
}

std::string BuildSnapshot() {
    std::string s = "{\"ready\":true";
    s += ",\"game\":" + Json(acc::game::TitleName());
    s += ",\"mod_version\":" + Json(std::string(acc::kModVersion) + "+bridge");

    if (!acc::game::IsKotor1()) {
        s += ",\"note\":\"state reads are KOTOR 1 only for now\"}";
        return s;
    }

    // World reads only once the player creature exists. Between character
    // creation's Play and the first area the server already has the area but
    // not the player yet, and reading the area name, party and target from
    // here every 250 ms kept the new game's load screen up for good (bisected
    // 2026-11-08: the bridge tick alone was enough, the rest of the mod not).
    // Menus and spoken lines still work: they don't go through this snapshot.
    if (!acc::engine::GetPlayerServerCreature()) {
        s += ",\"in_world\":false";
        void* mgr0 = acc::engine::GetGuiManager();
        void* panel0 = mgr0 ? acc::engine::GetForegroundPanel(mgr0) : nullptr;
        if (panel0) {
            s += ",\"screen\":" + Json(acc::engine::PanelKindName(
                                       acc::engine::IdentifyPanel(panel0)));
        }
        return s + "}";
    }

    char buf[256] = {0};
    void* area = acc::engine::GetCurrentArea();
    if (area && acc::engine::GetAreaDisplayName(area, buf, sizeof(buf)))
        s += ",\"area\":" + JsonGame(buf);
    buf[0] = 0;
    if (acc::engine::GetCurrentAreaResName(buf, sizeof(buf)))
        s += ",\"area_resref\":" + JsonGame(buf);

    PlayerFrame f = ReadPlayerFrame();
    if (f.ok) {
        s += ",\"position\":{\"x\":" + Num(f.pos.x) + ",\"y\":" + Num(f.pos.y) +
             ",\"z\":" + Num(f.pos.z) + "}";
    }
    if (f.yawOk) {
        float compass = acc::engine::EngineYawToCompass(f.yawDeg);
        s += ",\"facing\":{\"bearing\":" + Num(compass, 0) +
             ",\"direction\":" + Json(Dir8(compass)) + "}";
    }

    buf[0] = 0;
    if (acc::engine::GetActiveLeaderName(buf, sizeof(buf)))
        s += ",\"leader\":" + JsonGame(buf);

    // Party: the controlled creature plus the NPC members, each once. With a
    // companion as leader (Tab) the controlled creature is that companion,
    // and the member table can list it again.
    s += ",\"party\":[";
    bool first = true;
    uint32_t listed[13] = {0};
    int listedCount = 0;
    auto addMember = [&](void* obj) {
        if (!obj) return;
        const uint32_t h = acc::engine::GetObjectHandle(obj);
        for (int k = 0; k < listedCount; ++k) {
            if (h != 0 && listed[k] == h) return;
        }
        if (listedCount < 13) listed[listedCount++] = h;
        if (!first) s += ",";
        s += CreatureJson(obj);
        first = false;
    };
    addMember(acc::engine::GetPlayerServerCreature());
    uint32_t handles[12] = {0};
    int n = acc::engine::GetPartyMembers(handles, 12);
    for (int i = 0; i < n; ++i) {
        addMember(acc::engine::ResolveServerObjectHandle(handles[i]));
    }
    // The player character is in neither list while a companion leads (Tab):
    // find it in the area by its chargen name.
    char pcName[128] = {0};
    if (area && acc::engine::GetPlayerCharacterName(pcName, sizeof(pcName)) &&
        pcName[0]) {
        acc::engine::AreaObjectIterator it(area);
        while (void* obj = it.Next()) {
            if (acc::engine::GetObjectKind(obj) !=
                static_cast<int>(acc::engine::GameObjectKind::Creature)) {
                continue;
            }
            char nm[128] = {0};
            if (!acc::engine::GetObjectName(obj, nm, sizeof(nm)) ||
                _stricmp(nm, pcName) != 0) {
                continue;
            }
            addMember(obj);
            break;
        }
    }
    s += "]";

    s += ",\"in_combat\":";
    s += acc::engine::IsAnyPartyMemberInCombat() ? "true" : "false";

    void* mgr = acc::engine::GetGuiManager();
    void* panel = mgr ? acc::engine::GetForegroundPanel(mgr) : nullptr;
    using PK = acc::engine::PanelKind;
    PK kind = PK::Unknown;
    if (panel) {
        kind = acc::engine::IdentifyPanel(panel);
        s += ",\"screen\":" + Json(acc::engine::PanelKindName(kind));
    }
    const bool inDialog =
        kind == PK::DialogCinematic || kind == PK::DialogCinematicCopy ||
        kind == PK::DialogComputer || kind == PK::DialogComputerCamera ||
        kind == PK::DialogLetterbox1 || kind == PK::DialogLetterbox2 ||
        kind == PK::DialogLetterbox3;

    // Dialogue replies, when a conversation is waiting for a choice. The
    // NPC's own line is in /spoken; the player picks a reply with the number
    // keys (DirectInput scancodes 0x02..0x0A for 1..9).
    int replies = inDialog ? acc::engine::ReadDialogReplyCount() : 0;
    if (replies > 0 && replies < 20) {
        s += ",\"dialog_replies\":[";
        bool firstReply = true;
        for (int i = 0; i < replies; ++i) {
            char rbuf[1024] = {0};
            if (!acc::engine::ReadDialogReplyText(i, rbuf, sizeof(rbuf))) continue;
            if (!firstReply) s += ",";
            firstReply = false;
            s += "{\"number\":" + std::to_string(i + 1) + ",\"text\":" +
                 JsonGame(rbuf) + "}";
        }
        s += "]";
    }

    acc::narrated_target::Slot slot;
    if (acc::narrated_target::TryGet(slot) && !slot.isMapPin && slot.obj) {
        Vector p = slot.pos;
        acc::engine::GetObjectPosition(slot.obj, p);
        char hbuf[16];
        std::snprintf(hbuf, sizeof(hbuf), "0x%08x", slot.handle);
        s += ",\"target\":{\"name\":" + Json(SpokenName(slot.obj)) +
             ",\"handle\":" + Json(hbuf) +
             ",\"where\":" + RelativeJson(f, p) + "}";
    }

    s += "}";
    return s;
}

const char* CategoryKey(acc::filter::CycleCategory c) {
    using C = acc::filter::CycleCategory;
    switch (c) {
        case C::Door:       return "door";
        case C::Npc:        return "creature";
        case C::Container:  return "container";
        case C::Item:       return "item";
        case C::Landmark:   return "landmark";
        case C::Transition: return "transition";
        default:            return "other";
    }
}

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

// The open screen as the mod's menu navigation sees it: every entry of the
// focus chain the arrow keys walk, with the focused one marked. Only when
// that chain belongs to the foreground panel (it is kept after a panel
// closes).
std::string BuildMenu() {
    void* mgr = acc::engine::GetGuiManager();
    void* panel = mgr ? acc::engine::GetForegroundPanel(mgr) : nullptr;
    std::string s = "{";
    if (panel) {
        s += "\"screen\":" +
             Json(acc::engine::PanelKindName(acc::engine::IdentifyPanel(panel))) +
             ",";
    }
    namespace ch = acc::menus::chain;
    if (!panel || ch::g_chainPanel != panel || ch::g_chainCount <= 0) {
        return s + "\"open\":false}";
    }
    s += "\"open\":true,\"focused\":" + std::to_string(ch::g_chainIndex + 1) +
         ",\"count\":" + std::to_string(ch::g_chainCount) + ",\"entries\":[";
    const int n = ch::g_chainCount < 150 ? ch::g_chainCount : 150;
    for (int i = 0; i < n; ++i) {
        const ch::ChainEntry& e = ch::g_chain[i];
        char buf[512] = {0};
        if (e.virtualKind == 0 && e.control) {
            acc::menus::extract::FromControl(e.control, buf, sizeof(buf), panel);
        }
        if (i) s += ",";
        s += "{\"number\":" + std::to_string(i + 1) +
             ",\"text\":" + JsonGame(buf[0] ? buf : "?") +
             (i == ch::g_chainIndex ? ",\"focused\":true" : "") + "}";
    }
    return s + "]}";
}

uint32_t ParseHandle(const std::string& v) {
    if (v.empty()) return 0;
    return static_cast<uint32_t>(std::strtoul(v.c_str(), nullptr, 0));
}

std::string Param(const Job& job, const char* key) {
    auto it = job.params.find(key);
    return it == job.params.end() ? std::string() : it->second;
}

std::string RunAction(const Job& job) {
    std::string action = Param(job, "action");

    if (action == "say") {
        std::wstring w = Utf8ToWide(Param(job, "text"));
        if (w.empty()) return "{\"error\":\"text missing\"}";
        prism::Speak(w.c_str(), /*interrupt=*/true);
        return "{\"ok\":true}";
    }
    if (action == "key") {
        std::string sc = Param(job, "scancode");
        if (sc.empty()) return "{\"error\":\"scancode missing\"}";
        int code = static_cast<int>(std::strtol(sc.c_str(), nullptr, 0));
        bool ext = Param(job, "extended") == "1";
        acc::key_inject::Tap(code, ext);
        return "{\"ok\":true}";
    }

    if (!acc::game::IsKotor1())
        return "{\"error\":\"this action is KOTOR 1 only for now\"}";

    if (action == "walk") {
        PlayerFrame f = ReadPlayerFrame();
        if (!f.ok) return "{\"error\":\"no player position\"}";
        Vector dst = f.pos;
        dst.x = static_cast<float>(std::atof(Param(job, "x").c_str()));
        dst.y = static_cast<float>(std::atof(Param(job, "y").c_str()));
        bool ok = acc::guidance::WalkTo(dst);
        return ok ? "{\"ok\":true}" : "{\"error\":\"walk refused\"}";
    }

    uint32_t handle = ParseHandle(Param(job, "handle"));
    if (handle == 0) return "{\"error\":\"handle missing\"}";
    void* obj = acc::engine::ResolveServerObjectHandle(handle);
    if (!obj) return "{\"error\":\"no object with that handle\"}";

    if (action == "select") {
        acc::narrated_target::Stamp(obj, handle);
        std::string name = SpokenName(obj);
        if (Param(job, "quiet") != "1") {
            std::wstring w = Utf8ToWide(name);
            if (!w.empty()) prism::Speak(w.c_str(), /*interrupt=*/true);
        }
        return "{\"ok\":true,\"name\":" + Json(name) + "}";
    }
    if (action == "interact") {
        acc::narrated_target::Stamp(obj, handle);
        acc::interact::DispatchInteract(obj, handle, /*forceRadial=*/false);
        return "{\"ok\":true}";
    }
    if (action == "use") {
        bool ok = acc::guidance::UseObject(handle);
        return ok ? "{\"ok\":true}" : "{\"error\":\"use refused\"}";
    }
    return "{\"error\":\"unknown action\"}";
}

std::string RunJob(const Job& job) {
    if (job.path == "/nearby") {
        std::string r = Param(job, "radius");
        float radius = r.empty() ? 0.0f : static_cast<float>(std::atof(r.c_str()));
        return BuildNearby(radius);
    }
    if (job.path == "/do") return RunAction(job);
    if (job.path == "/overview") return BuildOverview();
    if (job.path == "/objects") return BuildObjects();
    if (job.path == "/rooms") return BuildRooms();
    if (job.path == "/menu") return BuildMenu();
    return "{\"error\":\"unknown job\"}";
}

// ---------------------------------------------------------------------------
// HTTP plumbing (socket thread)

std::string UrlDecode(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c == '+') {
            out.push_back(' ');
        } else if (c == '%' && i + 2 < s.size()) {
            char hex[3] = {s[i + 1], s[i + 2], 0};
            out.push_back(static_cast<char>(std::strtol(hex, nullptr, 16)));
            i += 2;
        } else {
            out.push_back(c);
        }
    }
    return out;
}

void ParseQuery(const std::string& q, std::map<std::string, std::string>& out) {
    size_t pos = 0;
    while (pos <= q.size()) {
        size_t amp = q.find('&', pos);
        if (amp == std::string::npos) amp = q.size();
        std::string pair = q.substr(pos, amp - pos);
        if (!pair.empty()) {
            size_t eq = pair.find('=');
            if (eq == std::string::npos) out[UrlDecode(pair)] = "";
            else out[UrlDecode(pair.substr(0, eq))] = UrlDecode(pair.substr(eq + 1));
        }
        pos = amp + 1;
    }
}

void SendResponse(SOCKET c, int status, const std::string& body) {
    const char* reason = status == 200 ? "OK"
                       : status == 403 ? "Forbidden"
                       : status == 404 ? "Not Found"
                       : status == 504 ? "Gateway Timeout"
                       : "Error";
    std::string head = "HTTP/1.0 " + std::to_string(status) + " " + reason +
                       "\r\nContent-Type: application/json; charset=utf-8"
                       "\r\nCache-Control: no-store"
                       "\r\nContent-Length: " + std::to_string(body.size()) +
                       "\r\nConnection: close\r\n\r\n";
    std::string all = head + body;
    const char* p = all.data();
    int left = static_cast<int>(all.size());
    while (left > 0) {
        int sent = send(c, p, left, 0);
        if (sent <= 0) break;
        p += sent;
        left -= sent;
    }
}

std::string QueueAndWait(const std::string& path,
                         const std::map<std::string, std::string>& params,
                         int* status) {
    auto job = std::make_shared<Job>();
    job->path = path;
    job->params = params;
    std::unique_lock<std::mutex> lock(g_mu);
    g_jobs.push_back(job);
    bool done = g_cv.wait_for(lock, std::chrono::milliseconds(kJobTimeoutMs),
                              [&] { return job->done; });
    if (!done) {
        *status = 504;
        return "{\"error\":\"the game did not handle the request in time "
               "(is it minimised or loading?)\"}";
    }
    *status = 200;
    return job->result;
}

void HandleClient(SOCKET c) {
    DWORD timeout = 3000;
    setsockopt(c, SOL_SOCKET, SO_RCVTIMEO,
               reinterpret_cast<const char*>(&timeout), sizeof(timeout));
    std::string req;
    char buf[2048];
    while (req.find("\r\n\r\n") == std::string::npos && req.size() < 16384) {
        int got = recv(c, buf, sizeof(buf), 0);
        if (got <= 0) break;
        req.append(buf, static_cast<size_t>(got));
    }
    size_t lineEnd = req.find("\r\n");
    if (lineEnd == std::string::npos) {
        SendResponse(c, 400, "{\"error\":\"bad request\"}");
        return;
    }
    std::string line = req.substr(0, lineEnd);
    size_t sp1 = line.find(' ');
    size_t sp2 = line.find(' ', sp1 + 1);
    if (sp1 == std::string::npos || sp2 == std::string::npos) {
        SendResponse(c, 400, "{\"error\":\"bad request line\"}");
        return;
    }
    std::string target = line.substr(sp1 + 1, sp2 - sp1 - 1);

    // Token check (header names are case-insensitive).
    const std::string lower =
        acc::strfmt::ToLowerAscii(req.substr(0, req.find("\r\n\r\n")));
    const std::string want = acc::strfmt::ToLowerAscii("x-kotor-bridge: " + g_token);
    if (lower.find(want) == std::string::npos) {
        SendResponse(c, 403, "{\"error\":\"missing or wrong X-Kotor-Bridge token\"}");
        return;
    }

    std::string path = target, query;
    size_t qm = target.find('?');
    if (qm != std::string::npos) {
        path = target.substr(0, qm);
        query = target.substr(qm + 1);
    }
    std::map<std::string, std::string> params;
    ParseQuery(query, params);

    if (path == "/health") {
        DWORD now = GetTickCount();
        std::lock_guard<std::mutex> lock(g_mu);
        DWORD age = g_lastTickMs ? now - g_lastTickMs : 0xFFFFFFFF;
        SendResponse(c, 200, "{\"ok\":true,\"game\":" +
                     Json(acc::game::TitleName()) +
                     ",\"mod_version\":" +
                     Json(std::string(acc::kModVersion) + "+bridge") +
                     ",\"last_tick_ms_ago\":" + std::to_string(age) + "}");
        return;
    }
    if (path == "/spoken") {
        uint32_t since = static_cast<uint32_t>(
            std::strtoul(params["since"].c_str(), nullptr, 10));
        int max = params["max"].empty() ? 50 : std::atoi(params["max"].c_str());
        if (max <= 0 || max > 300) max = 50;
        std::lock_guard<std::mutex> lock(g_mu);
        std::vector<const Spoken*> picked;
        for (auto it = g_spoken.rbegin(); it != g_spoken.rend(); ++it) {
            if (it->id <= since) break;
            picked.push_back(&*it);
            if (static_cast<int>(picked.size()) >= max) break;
        }
        DWORD now = GetTickCount();
        std::string body = "{\"next\":" + std::to_string(g_nextSpokenId - 1) +
                           ",\"lines\":[";
        for (size_t i = picked.size(); i-- > 0;) {
            const Spoken* sp = picked[i];
            body += "{\"id\":" + std::to_string(sp->id) +
                    ",\"seconds_ago\":" + Num((now - sp->ms) / 1000.0) +
                    ",\"text\":" + Json(sp->text) + "}";
            if (i) body += ",";
        }
        body += "]}";
        SendResponse(c, 200, body);
        return;
    }
    if (path == "/state") {
        std::lock_guard<std::mutex> lock(g_mu);
        DWORD now = GetTickCount();
        std::string body = g_snapshot;
        // Splice the snapshot age into the object.
        if (!body.empty() && body.back() == '}') {
            body.pop_back();
            body += ",\"snapshot_ms_ago\":" +
                    std::to_string(g_snapshotMs ? now - g_snapshotMs : 0) + "}";
        }
        SendResponse(c, 200, body);
        return;
    }
    if (path == "/nearby" || path == "/do" || path == "/overview" ||
        path == "/menu" || path == "/objects" || path == "/rooms") {
        int status = 200;
        std::string body = QueueAndWait(path, params, &status);
        SendResponse(c, status, body);
        return;
    }
    SendResponse(c, 404, "{\"error\":\"unknown path\"}");
}

void WriteConnectionFile() {
    char exePath[MAX_PATH] = {0};
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    char* slash = std::strrchr(exePath, '\\');
    if (!slash) return;
    strcpy_s(slash + 1, MAX_PATH - static_cast<size_t>(slash + 1 - exePath),
             "bridge.json");
    FILE* fp = nullptr;
    if (fopen_s(&fp, exePath, "wb") != 0 || !fp) {
        acclog::Write("Bridge", "could not write %s", exePath);
        return;
    }
    std::fprintf(fp, "{\"port\":%d,\"token\":\"%s\",\"pid\":%lu}\n", g_port,
                 g_token.c_str(), GetCurrentProcessId());
    fclose(fp);
    acclog::Write("Bridge", "connection file written: %s", exePath);
}

std::string MakeToken() {
    unsigned char bytes[16] = {0};
    if (BCryptGenRandom(nullptr, bytes, sizeof(bytes),
                        BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) {
        LARGE_INTEGER q;
        QueryPerformanceCounter(&q);
        for (int i = 0; i < 16; ++i)
            bytes[i] = static_cast<unsigned char>(
                (q.QuadPart >> ((i % 8) * 8)) ^ (GetTickCount() >> i) ^ (i * 151));
    }
    std::string t;
    for (unsigned char b : bytes) {
        char buf[3];
        std::snprintf(buf, sizeof(buf), "%02x", b);
        t += buf;
    }
    return t;
}

void ServerThread(SOCKET listener) {
    for (;;) {
        SOCKET c = accept(listener, nullptr, nullptr);
        if (c == INVALID_SOCKET) {
            Sleep(100);
            continue;
        }
        HandleClient(c);
        shutdown(c, SD_SEND);
        closesocket(c);
    }
}

void StartServer() {
    g_started = true;
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        acclog::Write("Bridge", "WSAStartup failed");
        return;
    }
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        acclog::Write("Bridge", "socket() failed: %d", WSAGetLastError());
        return;
    }
    // Refuse to share the port with another process.
    BOOL excl = TRUE;
    setsockopt(s, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
               reinterpret_cast<const char*>(&excl), sizeof(excl));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int port = 0;
    for (int i = 0; i < kPortAttempts; ++i) {
        addr.sin_port = htons(static_cast<u_short>(kFirstPort + i));
        if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0) {
            port = kFirstPort + i;
            break;
        }
    }
    if (!port || listen(s, 8) != 0) {
        acclog::Write("Bridge", "could not bind/listen on 127.0.0.1:%d..%d",
                      kFirstPort, kFirstPort + kPortAttempts - 1);
        closesocket(s);
        return;
    }
    g_port = port;
    g_token = MakeToken();
    WriteConnectionFile();
    std::thread(ServerThread, s).detach();
    acclog::Write("Bridge", "listening on http://127.0.0.1:%d/", port);
}

}  // namespace

void RecordSpoken(const char* text) {
    if (!text || !*text) return;
    std::string u = ToUtf8(text);
    std::lock_guard<std::mutex> lock(g_mu);
    g_spoken.push_back(Spoken{g_nextSpokenId++, GetTickCount(), std::move(u)});
    while (g_spoken.size() > kSpokenCapacity) g_spoken.pop_front();
}

void Tick() {
    if (!g_started) StartServer();

    DWORD now = GetTickCount();
    bool refresh = false;
    std::deque<std::shared_ptr<Job>> jobs;
    {
        std::lock_guard<std::mutex> lock(g_mu);
        g_lastTickMs = now;
        refresh = (now - g_snapshotMs) >= kSnapshotPeriodMs || !g_snapshotMs;
        jobs.swap(g_jobs);
    }

    if (refresh) {
        std::string snap = BuildSnapshot();
        std::lock_guard<std::mutex> lock(g_mu);
        g_snapshot = std::move(snap);
        g_snapshotMs = now;
    }

    if (jobs.empty()) return;
    for (auto& job : jobs) {
        std::string result = RunJob(*job);
        acclog::Write("Bridge", "%s action=%s -> %.120s", job->path.c_str(),
                      Param(*job, "action").c_str(), result.c_str());
        std::lock_guard<std::mutex> lock(g_mu);
        job->result = std::move(result);
        job->done = true;
    }
    g_cv.notify_all();
}

}  // namespace acc::bridge

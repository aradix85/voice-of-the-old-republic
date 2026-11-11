// Local bridge: /state (area, position, party, screen, dialogue replies,
// target) and /menu.

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include "bridge_internal.h"
#include "engine_area.h"
#include "engine_compass.h"
#include "engine_game.h"
#include "engine_manager.h"
#include "engine_panels.h"
#include "engine_player.h"
#include "engine_reads.h"
#include "menus_chain.h"
#include "menus_extract.h"
#include "narrated_target.h"
#include "party_status.h"

namespace acc::bridge {

namespace {

constexpr int kMaxPartyHandles = 12;
constexpr int kMaxMenuEntries  = 150;

void* ForegroundPanel() {
    void* mgr = acc::engine::GetGuiManager();
    return mgr ? acc::engine::GetForegroundPanel(mgr) : nullptr;
}

std::string ScreenField(void* panel) {
    if (!panel) return "";
    return ",\"screen\":" +
           Json(acc::engine::PanelKindName(acc::engine::IdentifyPanel(panel)));
}

bool IsDialogPanel(void* panel) {
    if (!panel) return false;
    using PK = acc::engine::PanelKind;
    const PK kind = acc::engine::IdentifyPanel(panel);
    return kind == PK::DialogCinematic || kind == PK::DialogCinematicCopy ||
           kind == PK::DialogComputer || kind == PK::DialogComputerCamera ||
           kind == PK::DialogLetterbox1 || kind == PK::DialogLetterbox2 ||
           kind == PK::DialogLetterbox3;
}

std::string CreatureJson(void* serverCreature) {
    std::string name = "?";
    char buf[128] = {0};
    if (acc::engine::GetObjectName(serverCreature, buf, sizeof(buf)) && buf[0])
        name = ToUtf8(buf);
    int hp = acc::engine::ReadCreatureCurrentHitPoints(serverCreature);
    int maxHp = acc::engine::ReadCreatureMaxHitPoints(serverCreature);
    std::string s = "{\"name\":" + Json(name) + ",\"hp\":" + std::to_string(hp) +
                    ",\"max_hp\":" + std::to_string(maxHp) +
                    ",\"handle\":" + HandleJson(acc::engine::GetObjectHandle(serverCreature));
    int security = 0;
    if (acc::party_status::SecurityOf(serverCreature, security))
        s += ",\"security\":" + std::to_string(security);
    return s + "}";
}

// The controlled creature plus the other members, each once. With a companion
// as leader (Tab) the controlled creature is that companion, and
// GetPartyMembers lists it again.
std::string PartyJson() {
    std::string s;
    std::vector<uint32_t> listed;
    auto add = [&](void* obj) {
        if (!obj) return;
        const uint32_t h = acc::engine::GetObjectHandle(obj);
        if (h != 0 && std::find(listed.begin(), listed.end(), h) != listed.end()) return;
        listed.push_back(h);
        if (!s.empty()) s += ",";
        s += CreatureJson(obj);
    };
    add(acc::engine::GetPlayerServerCreature());
    uint32_t handles[kMaxPartyHandles] = {0};
    const int n = acc::engine::GetPartyMembers(handles, kMaxPartyHandles);
    for (int i = 0; i < n; ++i) add(acc::engine::ResolveServerObjectHandle(handles[i]));
    return "[" + s + "]";
}

// The replies of a conversation waiting for a choice. The NPC's own line is
// in /spoken; the player picks a reply with the number keys (DirectInput
// scancodes 0x02..0x0A for 1..9).
std::string DialogRepliesField() {
    const int replies = acc::engine::ReadDialogReplyCount();
    if (replies <= 0 || replies >= 20) return "";
    std::string s;
    for (int i = 0; i < replies; ++i) {
        char rbuf[1024] = {0};
        if (!acc::engine::ReadDialogReplyText(i, rbuf, sizeof(rbuf))) continue;
        if (!s.empty()) s += ",";
        s += "{\"number\":" + std::to_string(i + 1) + ",\"text\":" + JsonGame(rbuf) + "}";
    }
    return ",\"dialog_replies\":[" + s + "]";
}

std::string TargetField(const PlayerFrame& f) {
    acc::narrated_target::Slot slot;
    if (!acc::narrated_target::TryGet(slot) || slot.isMapPin || !slot.obj) return "";
    Vector p = slot.pos;
    acc::engine::GetObjectPosition(slot.obj, p);
    return ",\"target\":{\"name\":" + Json(SpokenName(slot.obj)) +
           ",\"handle\":" + HandleJson(slot.handle) +
           ",\"where\":" + RelativeJson(f, p) + "}";
}

}  // namespace

std::string BuildSnapshot() {
    std::string s = "{\"ready\":true," + GameFields();
    if (!acc::game::IsKotor1())
        return s + ",\"note\":\"state reads are KOTOR 1 only for now\"}";

    void* panel = ForegroundPanel();
    // World reads only once the player creature exists. Between character
    // creation's Play and the first area the server already has the area but
    // not the player yet, and reading the area name, party and target then
    // kept the new game's load screen up for good.
    if (!acc::engine::GetPlayerServerCreature())
        return s + ",\"in_world\":false" + ScreenField(panel) + "}";

    char buf[256] = {0};
    void* area = acc::engine::GetCurrentArea();
    if (area && acc::engine::GetAreaDisplayName(area, buf, sizeof(buf)))
        s += ",\"area\":" + JsonGame(buf);
    buf[0] = 0;
    if (acc::engine::GetCurrentAreaResName(buf, sizeof(buf)))
        s += ",\"area_resref\":" + JsonGame(buf);

    const PlayerFrame f = ReadPlayerFrame();
    if (f.ok) {
        s += ",\"position\":{\"x\":" + Num(f.pos.x) + ",\"y\":" + Num(f.pos.y) +
             ",\"z\":" + Num(f.pos.z) + "}";
    }
    if (f.yawOk) {
        const float compass = acc::engine::EngineYawToCompass(f.yawDeg);
        s += ",\"facing\":{\"bearing\":" + Num(compass, 0) +
             ",\"direction\":" + Json(Dir8(compass)) + "}";
    }
    buf[0] = 0;
    if (acc::engine::GetActiveLeaderName(buf, sizeof(buf)))
        s += ",\"leader\":" + JsonGame(buf);

    s += ",\"party\":" + PartyJson();
    s += ",\"in_combat\":";
    s += acc::engine::IsAnyPartyMemberInCombat() ? "true" : "false";
    s += ScreenField(panel);
    if (IsDialogPanel(panel)) s += DialogRepliesField();
    s += TargetField(f);
    return s + "}";
}

// The open screen as the mod's menu navigation sees it: every entry of the
// focus chain the arrow keys walk, with the focused one marked. Only when
// that chain belongs to the foreground panel (it is kept after a panel
// closes).
std::string BuildMenu() {
    void* panel = ForegroundPanel();
    namespace ch = acc::menus::chain;
    if (!panel || ch::g_chainPanel != panel || ch::g_chainCount <= 0)
        return "{\"open\":false" + ScreenField(panel) + "}";

    std::string s = "{\"open\":true" + ScreenField(panel) +
                    ",\"focused\":" + std::to_string(ch::g_chainIndex + 1) +
                    ",\"count\":" + std::to_string(ch::g_chainCount) + ",\"entries\":[";
    const int n = (std::min)(ch::g_chainCount, kMaxMenuEntries);
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

}  // namespace acc::bridge

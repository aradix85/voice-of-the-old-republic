// Local bridge: the /state snapshot (area, position, party, screen, dialogue replies, target) and /menu.

#include "bridge_internal.h"

namespace acc::bridge {

namespace {

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

}  // namespace

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

}  // namespace acc::bridge

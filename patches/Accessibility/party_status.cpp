// Party status — see party_status.h.
#include "party_status.h"

#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <set>
#include <string>
#include <vector>

#include "engine_area.h"
#include "engine_game.h"
#include "engine_levelup.h"
#include "engine_offsets.h"
#include "engine_offsets_select.h"
#include "engine_panels.h"
#include "engine_player.h"
#include "engine_reads.h"
#include "hotkeys.h"
#include "log.h"
#include "speech.h"
#include "strings.h"
#include "transitions.h"

namespace acc::party_status {

namespace {

struct Member {
    void*    obj;
    uint32_t handle;
};

// CSWSCreatureStats skill fields, KOTOR 1 (1.03 address DB): skill_ranks is
// a pointer to the per-skill base ranks, int_mod the Intelligence modifier.
const size_t kStatsSkillRanksOffset = acc::off::Kotor1Only(0x168);  // byte*
const size_t kStatsIntModOffset     = acc::off::Kotor1Only(0xf0);   // byte
constexpr int kSkillSecurity = 6;  // skills.2da row

constexpr DWORD kLevelCheckPeriodMs = 1000;

std::set<uint32_t> g_toldCanLevel;  // members already told, until it clears
DWORD              g_lastLevelCheck = 0;

std::vector<Member> PartyMembers() {
    std::vector<Member> out;
    void* pc = acc::engine::GetPlayerServerCreature();
    if (pc) out.push_back(Member{pc, acc::engine::GetObjectHandle(pc)});
    uint32_t handles[12] = {0};
    const int n = acc::engine::GetPartyMembers(handles, 12);
    for (int i = 0; i < n; ++i) {
        void* obj = acc::engine::ResolveServerObjectHandle(handles[i]);
        if (obj && obj != pc) out.push_back(Member{obj, handles[i]});
    }
    return out;
}

std::string NameOf(void* obj) {
    char buf[128] = {0};
    if (acc::engine::GetObjectName(obj, buf, sizeof(buf)) && buf[0]) return buf;
    return "?";
}

bool ReadSecurity(void* creature, int& out) {
    if (!creature || !acc::game::IsKotor1() ||
        !acc::off::Ok(kStatsSkillRanksOffset) ||
        !acc::off::Ok(kStatsIntModOffset)) {
        return false;
    }
    __try {
        auto* base = reinterpret_cast<unsigned char*>(creature);
        auto* stats = *reinterpret_cast<unsigned char**>(
            base + kCreatureStatsPointerOffset);
        if (!stats) return false;
        auto* ranks = *reinterpret_cast<unsigned char**>(
            stats + kStatsSkillRanksOffset);
        if (!ranks) return false;
        const int rank = ranks[kSkillSecurity];
        const int mod  = *reinterpret_cast<signed char*>(stats + kStatsIntModOffset);
        if (rank > 60 || mod < -6 || mod > 12) return false;  // not these fields
        out = rank + mod;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool InWorldAndFree() {
    Vector unused;
    if (!acc::engine::GetPlayerPosition(unused)) return false;
    return !acc::engine::IsForegroundUiBlocking();
}

}  // namespace

void PollHotkey() {
    if (!acc::hotkeys::Pressed(acc::hotkeys::Action::PartyStatusAnnounce)) return;
    if (!InWorldAndFree()) return;
    std::string text;
    for (const Member& m : PartyMembers()) {
        const int hp    = acc::engine::ReadCreatureCurrentHitPoints(m.obj);
        const int maxHp = acc::engine::ReadCreatureMaxHitPoints(m.obj);
        char buf[192];
        std::snprintf(buf, sizeof(buf),
                      acc::strings::Get(acc::strings::Id::FmtPartyMemberHp),
                      NameOf(m.obj).c_str(), hp, maxHp);
        std::string entry = buf;
        if (acc::engine_levelup::CreatureCanLevelUp(m.obj)) {
            entry += ", ";
            entry += acc::strings::Get(acc::strings::Id::PartyCanLevelUp);
        }
        if (!text.empty()) text += "; ";
        text += entry;
    }
    if (text.empty()) return;
    acc::speech::Speak(text.c_str(), /*interrupt=*/true);
    acclog::Write("PartyStatus", "Ctrl+H: %s", text.c_str());
}

void Tick() {
    const DWORD now = GetTickCount();
    if (now - g_lastLevelCheck < kLevelCheckPeriodMs) return;
    g_lastLevelCheck = now;
    if (!InWorldAndFree()) return;
    if (acc::engine::HasActiveDialogPanel()) return;
    if (acc::transitions::IsWorldSpeechGated()) return;  // combat, loading
    for (const Member& m : PartyMembers()) {
        if (!acc::engine_levelup::CreatureCanLevelUp(m.obj)) {
            g_toldCanLevel.erase(m.handle);  // tell again at the next level
            continue;
        }
        if (!g_toldCanLevel.insert(m.handle).second) continue;
        char buf[192];
        std::snprintf(buf, sizeof(buf),
                      acc::strings::Get(acc::strings::Id::FmtCanLevelUpNotice),
                      NameOf(m.obj).c_str());
        acc::speech::Speak(buf, /*interrupt=*/false);
        acclog::Write("PartyStatus", "level-up notice: %s", buf);
    }
}

bool SecurityOf(void* serverCreature, int& outScore) {
    return ReadSecurity(serverCreature, outScore);
}

bool BestSecurity(std::string& outName, int& outScore) {
    bool found = false;
    for (const Member& m : PartyMembers()) {
        int score = 0;
        if (!ReadSecurity(m.obj, score)) continue;
        if (!found || score > outScore) {
            outScore = score;
            outName  = NameOf(m.obj);
            found    = true;
        }
    }
    return found;
}

}  // namespace acc::party_status

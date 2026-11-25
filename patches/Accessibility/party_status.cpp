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
#include "health_bands.h"
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
// a pointer to the per-skill base ranks, wis_mod the Wisdom modifier (the
// key ability of Security in skills.2da).
const size_t kStatsSkillRanksOffset = acc::off::Kotor1Only(0x168);  // byte*
const size_t kStatsWisModOffset     = acc::off::Kotor1Only(0xf2);   // byte

// KOTOR 2 asks the engine: CSWSCreatureStats::GetSkillRank(skill, versus,
// baseOnly), __thiscall, a signed char back. With baseOnly 0 it adds the
// skill's key ability modifier (Intelligence for Security in KOTOR 2), effects
// and the skill feats to the rank, the number the game itself rolls with.
const uintptr_t kAddrCSWSCreatureStatsGetSkillRank = acc::addr::Kotor2Only(0x006B7BB0);
typedef char (__thiscall* PFN_GetSkillRank)(void* stats, unsigned char skill,
                                            uint32_t versus, int baseOnly);

constexpr DWORD kLevelCheckPeriodMs = 1000;

std::set<uint32_t> g_toldCanLevel;  // members already told, until it clears
DWORD              g_lastLevelCheck = 0;

constexpr DWORD kHealthCheckPeriodMs = 250;

acc::combat::HealthWarnings g_healthWarnings;
DWORD                       g_lastHealthCheck = 0;

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

bool ReadSecurityK2(void* creature, int& out) {
    if (!acc::addr::Ok(kAddrCSWSCreatureStatsGetSkillRank)) return false;
    __try {
        auto* base = reinterpret_cast<unsigned char*>(creature);
        void* stats = *reinterpret_cast<void**>(base + kCreatureStatsPointerOffset);
        if (!stats) return false;
        out = reinterpret_cast<PFN_GetSkillRank>(kAddrCSWSCreatureStatsGetSkillRank)(
            stats, static_cast<unsigned char>(kSkillSecurity), kInvalidObjectId, 0);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool ReadSecurity(void* creature, int& out) {
    if (!creature) return false;
    if (acc::game::IsKotor2()) return ReadSecurityK2(creature, out);
    if (!acc::off::Ok(kStatsSkillRanksOffset) || !acc::off::Ok(kStatsWisModOffset)) {
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
        const int mod  = *reinterpret_cast<signed char*>(stats + kStatsWisModOffset);
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

std::string MembersBehind(float beyondM, float* outLeftM) {
    std::string names;
    float left = 0.0f;
    void* lead = acc::engine::GetPlayerServerCreature();
    Vector leadPos;
    if (!lead || !acc::engine::GetObjectPosition(lead, leadPos)) return names;
    for (const Member& m : PartyMembers()) {
        Vector pos;
        if (m.obj == lead || !acc::engine::GetObjectPosition(m.obj, pos)) continue;
        const float d = DistXY(pos, leadPos);
        if (d <= beyondM) continue;
        left += d;
        if (!names.empty()) names += ", ";
        names += NameOf(m.obj);
    }
    if (outLeftM) *outLeftM = left;
    return names;
}

std::string HealthLine(bool withLevelUp) {
    std::string text;
    for (const Member& m : PartyMembers()) {
        const int hp    = acc::engine::ReadCreatureCurrentHitPoints(m.obj);
        const int maxHp = acc::engine::ReadCreatureMaxHitPoints(m.obj);
        char buf[192];
        if (hp <= 0) {
            // The engine keeps counting below zero: "-11 of 42" is a member
            // who lies down until the fight is over.
            std::snprintf(buf, sizeof(buf),
                          acc::strings::Get(acc::strings::Id::FmtPartyMemberDown),
                          NameOf(m.obj).c_str());
        } else {
            std::snprintf(buf, sizeof(buf),
                          acc::strings::Get(acc::strings::Id::FmtPartyMemberHp),
                          NameOf(m.obj).c_str(), hp, maxHp);
        }
        std::string entry = buf;
        if (withLevelUp && acc::engine_levelup::CreatureCanLevelUp(m.obj)) {
            entry += ", ";
            entry += acc::strings::Get(acc::strings::Id::PartyCanLevelUp);
        }
        if (!text.empty()) text += "; ";
        text += entry;
    }
    return text;
}

bool AnyoneDown() {
    for (const Member& m : PartyMembers()) {
        if (acc::engine::ReadCreatureCurrentHitPoints(m.obj) <= 0) return true;
    }
    return false;
}

void PollHotkey() {
    if (!acc::hotkeys::Pressed(acc::hotkeys::Action::PartyStatusAnnounce)) return;
    if (!InWorldAndFree()) return;
    const std::string text = HealthLine(/*withLevelUp=*/true);
    if (text.empty()) return;
    acc::speech::Speak(text.c_str(), /*interrupt=*/true);
    acclog::Write("PartyStatus", "Ctrl+H: %s", text.c_str());
}

void TickHealthWarnings(bool inCombat) {
    if (!inCombat) {
        g_healthWarnings.Clear();
        return;
    }
    const DWORD now = GetTickCount();
    if (now - g_lastHealthCheck < kHealthCheckPeriodMs) return;
    g_lastHealthCheck = now;
    for (const Member& m : PartyMembers()) {
        const int hp    = acc::engine::ReadCreatureCurrentHitPoints(m.obj);
        const int maxHp = acc::engine::ReadCreatureMaxHitPoints(m.obj);
        const int band  = g_healthWarnings.Sank(m.handle, hp, maxHp);
        if (band == 0) continue;
        char buf[192];
        std::snprintf(buf, sizeof(buf),
                      acc::strings::Get(acc::strings::Id::FmtHealthSank),
                      NameOf(m.obj).c_str(), hp, maxHp);
        // A quarter is urgent enough to cut the blow-by-blow lines off.
        acc::speech::Speak(buf, /*interrupt=*/band == 2);
        acclog::Write("PartyStatus", "health warning, band %d: %s", band, buf);
    }
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

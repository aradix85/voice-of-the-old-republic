// A walk that leaves the area waits for the party. The engine refuses an
// area transition while a companion is not near ("You must gather your party
// before venturing forth"), and a long walk by Enter outruns the companions.
// So the leader stops kHoldBeforeM short of the target until everyone is
// within kGatheredM, and then goes on. It goes on without them, and says so,
// once nobody behind has come closer for kStillMaxMs (a companion that is
// stuck, or left behind in solo mode), and after kHoldMaxMs at the latest: a
// companion that cannot come is the engine's to report.

#include "guidance_approach_internal.h"

#include <string>

#include "guidance_autowalk.h"
#include "log.h"
#include "party_status.h"
#include "speech.h"
#include "strings.h"

namespace acc::guidance {

namespace {

constexpr float kHoldBeforeM = 8.0f;
constexpr float kGatheredM   = 7.0f;
constexpr DWORD kHoldMaxMs   = 20000;
constexpr DWORD kStillMaxMs  = 3000;
constexpr float kCloserM     = 0.5f;

void SayAboutParty(acc::strings::Id fmt, const std::string& names, char* line,
                   size_t cap) {
    std::snprintf(line, cap, acc::strings::Get(fmt), names.c_str());
    acc::speech::Speak(line, /*interrupt=*/false);
}

}  // namespace

bool TickPartyHold(const Vector& pos, DWORD now) {
    if (!g_st.leavesArea) return false;
    Vector tgt;
    if (!ResolveTargetPos(tgt)) return false;
    const bool holding = g_st.holdSince != 0;
    if (!holding && DistXY(tgt, pos) > kHoldBeforeM) return false;

    float left = 0.0f;
    const std::string behind = acc::party_status::MembersBehind(kGatheredM, &left);
    char line[192];
    if (!holding) {
        if (behind.empty()) {
            g_st.leavesArea = false;  // together at the end: nothing owed
            return false;
        }
        CancelMovement();
        g_st.holdSince    = now ? now : 1;
        g_st.holdLeftM    = left;
        g_st.holdCloserAt = now;
        SayAboutParty(acc::strings::Id::FmtWaitingForParty, behind, line, sizeof(line));
        acclog::Write("Approach", "party hold %.1fm before [%s] -> [%s]",
                      DistXY(tgt, pos), g_st.name, line);
        return true;
    }
    const char* why = "everyone is here";
    if (!behind.empty()) {
        if (left < g_st.holdLeftM - kCloserM) {
            g_st.holdLeftM    = left;
            g_st.holdCloserAt = now;
        }
        const bool still = now - g_st.holdCloserAt >= kStillMaxMs;
        if (!still && now - g_st.holdSince < kHoldMaxMs) return true;
        why = still ? "nobody is coming closer" : "gave up waiting";
        SayAboutParty(acc::strings::Id::FmtWalkingOnWithout, behind, line, sizeof(line));
    }

    acclog::Write("Approach", "party hold over after %lums (%s): walking on to [%s]",
                  static_cast<unsigned long>(now - g_st.holdSince), why, g_st.name);
    g_st.leavesArea = false;
    g_st.holdSince  = 0;
    WalkTo(tgt);
    MarkWalkStart();
    RestartStallWindow();
    return true;
}

}  // namespace acc::guidance

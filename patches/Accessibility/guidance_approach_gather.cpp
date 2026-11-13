// A walk that leaves the area waits for the party. The engine refuses an
// area transition while a companion is not near ("You must gather your party
// before venturing forth"), and a long walk by Enter outruns the companions.
// So the leader stops kHoldBeforeM short of the target until everyone is
// within kGatheredM, and then goes on. After kHoldMaxMs it goes on
// regardless: a companion that cannot come is the engine's to report.

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

}  // namespace

bool TickPartyHold(const Vector& pos, DWORD now) {
    if (!g_st.leavesArea) return false;
    Vector tgt;
    if (!ResolveTargetPos(tgt)) return false;
    const bool holding = g_st.holdSince != 0;
    if (!holding && DistXY(tgt, pos) > kHoldBeforeM) return false;

    const std::string behind = acc::party_status::MembersBehind(kGatheredM);
    if (!holding) {
        if (behind.empty()) {
            g_st.leavesArea = false;  // together at the end: nothing owed
            return false;
        }
        CancelMovement();
        g_st.holdSince = now ? now : 1;
        char line[192];
        std::snprintf(line, sizeof(line),
                      acc::strings::Get(acc::strings::Id::FmtWaitingForParty),
                      behind.c_str());
        acc::speech::Speak(line, /*interrupt=*/false);
        acclog::Write("Approach", "party hold %.1fm before [%s] -> [%s]",
                      DistXY(tgt, pos), g_st.name, line);
        return true;
    }
    if (!behind.empty() && now - g_st.holdSince < kHoldMaxMs) return true;

    acclog::Write("Approach", "party hold over after %lums (%s): walking on to [%s]",
                  static_cast<unsigned long>(now - g_st.holdSince),
                  behind.empty() ? "everyone is here" : "gave up waiting", g_st.name);
    g_st.leavesArea = false;
    g_st.holdSince  = 0;
    WalkTo(tgt);
    MarkWalkStart();
    RestartStallWindow();
    return true;
}

}  // namespace acc::guidance

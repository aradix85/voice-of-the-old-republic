// The swoop race's clock and finish: the race time from the world clock,
// whether the bike wrecked at the finish, and the end-of-race line.

#include "minigame_swoop_race_internal.h"

#include "engine_app.h"
#include "engine_area.h"
#include "engine_game.h"
#include "engine_addr.h"
#include "log.h"
#include "speech.h"
#include "strings.h"

namespace acc::swoop_race {

namespace {

// ----- Race timer: engine world clock − NWScript start-stamp globals -----
//
// KOTOR's swoop timer is NWScript-driven: each track's heartbeat stamps the
// race START time into the global-number table (MIN_TIME_HOUR/MIN/SEC/MIL,
// the last in centiseconds) at the "Go!" signal, then the displayed clock is
// (current world time − that stamp). No engine struct holds elapsed race time
// (CSWMiniGame / CSWMiniPlayer are pure geometry), so we reproduce the script:
//
//   server  = *AppManager + 0x8                          (CServerExoApp)
//   timer   = CServerExoApp::GetWorldTimer(server)        (CWorldTimer)
//   GetWorldTime(timer,&day,&nowMs)                       nowMs = ms-of-day
//   mph     = timer->minutes_per_hour  (+0x38, byte)      (compressed calendar)
//   sH/sM/sS/sC = ReadGlobalNumber("MIN_TIME_*")          start-stamp parts
//   startMs = (((sH*mph + sM)*60 + sS)*1000) + sC*10
//   elapsedMs = nowMs − startMs
//
// GetWorldTimeHour/Minute divide by minutes_per_hour, so the H/M/S split is
// NOT a normal clock — we reassemble through mph. Addresses from
// ExecuteCommandGetTimeHour / GetTimeMillisecond and the CWorldTimer
// accessors. The start stamp is constant for the race, so it's cached on
// first valid read.
//
// KOTOR 2 uses the SAME script scheme — its three swoop tracks (211TEL, 371NAR,
// 510OND, the Type==1 minigame areas) stamp MIN_TIME_HOUR/MIN/SEC/MIL from their
// own heartbeat.ncs, exactly as KOTOR 1's do — so this whole reader ports, with
// only the two engine addresses changing:
//   * GetWorldTimer @0x0051C370 — `[this+4]` then `[internal+0x10048]`, the same
//     internal slot as KOTOR 1; cross-checked against the two call sites that
//     inline it (0x0051D790 / 0x0051E170) and hand the result straight to
//     GetWorldTime.
//   * GetWorldTime @0x0051AD20 — instruction-for-instruction KOTOR 1's: paused
//     check at [this+0x24], the fast path copying [this+0x28] to *outDay and
//     [this+0x2c] to *outMs, the slow path dividing the snapshot by 1000 and
//     [this+0x3c]. Four confirmed field offsets in a row is what makes
//     minutes_per_hour Same() below rather than a guess — and it has its own
//     witness anyway: K2's hour/minute split @0x0051AB40 reads it with the same
//     `movzx ecx, byte ptr [reg+0x38]` KOTOR 1's GetWorldTimeHour uses.
const uintptr_t kAddrCServerExoAppGetWorldTimer =
    acc::addr::Pick(0x004aede0, 0x0051C370);
const uintptr_t kAddrCWorldTimerGetWorldTime =
    acc::addr::Pick(0x004ade40, 0x0051AD20);
const size_t    kWorldTimerMinutesPerHourOffset     = acc::off::Same(0x38);  // byte

typedef void* (__thiscall* PFN_GetWorldTimer)(void* server);
typedef void  (__thiscall* PFN_GetWorldTime)(void* timer, uint32_t* outDay,
                                             uint32_t* outMs);

// Finish detection. The engine ZEROES the speed envelope (min_speed = max_speed
// = 0) the instant the bike crosses the finish line and race control ends,
// while the bike still coasts at ~120 u/s. So the last tick with a live
// envelope (max_speed > 0) IS the finish crossing: the elapsed is updated
// every active tick and the zero-transition freezes it there. (The speed peak
// is not the finish: a slow run that peaked early would report a better
// time.)
constexpr float kRaceTimeMaxSeconds = 600.0f;   // implausible-read ceiling
constexpr long  kRaceStartFreshMaxMs = 5000;    // start-stamp freshness window
// Race-end announce: fire once the finish is crossed (envelope zeroed) on a bike
// that actually raced (running envelope cleared kRaceMinTopSpeed). kTerminalStop
// is a fallback trigger for an abnormal end where the envelope never zeroes and
// the bike just coasts below it. Rock hits reset speed to the gear minimum
// (>=35), never this low, so the fallback won't trip mid-race.
constexpr float kTerminalStopSpeed  = 10.0f;
constexpr float kRaceMinTopSpeed    = 40.0f;

// Current world-clock time-of-day in ms (the value GetTime* read) + the
// calendar's minutes-per-hour. False on any null link / SEH fault.
bool ReadWorldClockMs(void* server, uint32_t& outMs, uint32_t& outMph) {
    if (!server) return false;
    __try {
        auto getTimer = reinterpret_cast<PFN_GetWorldTimer>(
            kAddrCServerExoAppGetWorldTimer);
        void* timer = getTimer(server);
        if (!timer) return false;
        uint32_t day = 0, ms = 0;
        auto getTime = reinterpret_cast<PFN_GetWorldTime>(
            kAddrCWorldTimerGetWorldTime);
        getTime(timer, &day, &ms);
        uint32_t mph = *(reinterpret_cast<unsigned char*>(timer) +
                         kWorldTimerMinutesPerHourOffset);
        if (mph == 0) mph = 2;  // KOTOR default; guards a bad read
        outMs  = ms;
        outMph = mph;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// At the track's own race-end flag: finish or wreck? Both write MIN_RACE_GEAR
// = 6. The Citadel heartbeat (heartbeat.ncs, disassembled) ends the race when
// the bike's y passes 6000 (the line), when the hit-point percentage in the
// racedialog local is <= 0, or on a menu quit; for the latter two it stamps
// 59 / 59 / 99 into the track's time globals (`<MODULE>_SWOOP_MIN` / `_SEC` /
// `_MSEC`, module = the area resref, "211TEL" on Citadel) where a finish
// stores the real time. That stamp is the decision here, with a zero
// hit-point read as the fallback, so a wreck is not announced as a finish
// time. Names for the other two tracks follow the same pattern by
// assumption; a miss reads -1 and the fallback decides, and the log line
// shows which happened.
void DecideWreckAtFinish(void* miniGame) {
    void* player = SafeReadPtr(miniGame, kMiniGamePlayerOffset);
    const int hp = player ? static_cast<int>(SafeReadU32(player, kFollowerHpOffset))
                          : -1;

    char area[96] = "";
    if (!acc::engine::GetCurrentAreaResName(area, sizeof(area))) area[0] = '\0';
    for (char* p = area; *p; ++p) {
        if (*p >= 'a' && *p <= 'z') *p = static_cast<char>(*p - 'a' + 'A');
    }
    char name[128];
    int stamp[3] = { -1, -1, -1 };
    const char* const kSuffix[3] = { "_SWOOP_MIN", "_SWOOP_SEC", "_SWOOP_MSEC" };
    if (area[0]) {
        for (int i = 0; i < 3; ++i) {
            std::snprintf(name, sizeof(name), "%s%s", area, kSuffix[i]);
            stamp[i] = acc::engine::ReadGlobalNumber(name);
        }
    }
    const bool forfeit_stamp = stamp[0] == 59 && stamp[1] == 59 && stamp[2] == 99;
    const bool hp_zero       = hp == 0;
    g_state.wrecked = forfeit_stamp || hp_zero;
    acclog::Write("SwoopRace",
                  "race end: %s (area=%s stamp=%d:%d.%d hp=%d)",
                  g_state.wrecked ? "WRECK" : "finish",
                  area[0] ? area : "?", stamp[0], stamp[1], stamp[2], hp);
}

}  // namespace

// Per tick while the latch is alive: detect the finish, cache the start
// stamp once, and keep the elapsed current until the finish freezes it.
void TickRaceTimer(void* miniGame) {
    void* player = SafeReadPtr(miniGame, kMiniGamePlayerOffset);
    if (!player) return;
    const float speed    = SafeReadFloat(player, kFollowerSpeedOffset);
    const float maxSpeed = SafeReadFloat(player, kMiniPlayerMaxSpeedOffset);

    // Finish crossing = the speed envelope just zeroed (maxSpeed -> 0) on a bike
    // that reached a real gear; see the finish-detection note above
    // kRaceTimeMaxSeconds. Announce here, in the clear air right after the
    // finish while the bike coasts, using race_time_seconds frozen at the last
    // live-envelope tick.
    //
    // Fallback: an abnormal end where the envelope never zeroes but the bike has
    // actually RACED and coasts below kTerminalStopSpeed. CRITICAL: the fallback
    // is gated on peak ACTUAL speed, not the envelope peak. The envelope is
    // already 70 the instant gear 1 engages at launch, so gating the fallback on
    // it would let the launch ramp (speed climbing through [1..10]) fire the
    // speed<10 trigger and end the race at ~0 s. Actual peak speed only clears
    // kRaceMinTopSpeed once the bike is genuinely fast, by which point speed is
    // well above kTerminalStopSpeed. (Checked before the launch guard below
    // because at the finish speed is still high but maxSpeed is already 0.)
    if (g_state.have_race_time && !g_state.race_time_announced) {
        // The tracks' own finish signal: both games' heartbeat.ncs writes
        // MIN_RACE_GEAR = 6 when the bike crosses the line (see the gear-source
        // note at kGearGlobalName). Primary on both games.
        const bool script_finished =
            acc::engine::ReadGlobalNumber(kGearGlobalName) == 6;
        const bool envelope_finished =
            g_state.race_max_speed > kRaceMinTopSpeed && maxSpeed <= 0.0f;
        // The coast-to-a-stop fallback is KOTOR 1 only. On KOTOR 2 a collision
        // legitimately stops the bike dead mid-race (the hit script zeroes the
        // acceleration and the engine bounces it back), which would announce a
        // false race end at the first bar; the script flag above covers K2's
        // finish.
        const bool coasted_to_stop =
            !acc::game::IsKotor2() &&
            g_state.race_peak_speed > kRaceMinTopSpeed && speed < kTerminalStopSpeed;
        if (script_finished || envelope_finished || coasted_to_stop) {
            acclog::Write("SwoopRace", "finish: script=%d envelope=%d coast=%d",
                          script_finished ? 1 : 0, envelope_finished ? 1 : 0,
                          coasted_to_stop ? 1 : 0);
            // KOTOR 2 writes the same flag for a wreck; K1 cannot wreck.
            if (acc::game::IsKotor2()) DecideWreckAtFinish(miniGame);
            SpeakRaceEndMessage();
            return;
        }
    }

    if (speed <= 1.0f) return;     // countdown / not launched yet
    if (maxSpeed <= 0.0f) return;  // race already ended — envelope cleared

    void* server = acc::engine::GetServerApp();
    if (!server) return;

    // Cache the fixed start stamp once the heartbeat has written THIS race's
    // value. MIN_TIME_* persists between races, so a stale previous-race stamp
    // would read as a huge initial elapsed — defer until it's fresh (a
    // just-stamped start gives a sub-second elapsed at launch).
    if (!g_state.have_start_ms) {
        uint32_t nowMs = 0, mph = 2;
        if (!ReadWorldClockMs(server, nowMs, mph)) return;
        using acc::engine::ReadGlobalNumber;
        const int sH = ReadGlobalNumber("MIN_TIME_HOUR");
        const int sM = ReadGlobalNumber("MIN_TIME_MIN");
        const int sS = ReadGlobalNumber("MIN_TIME_SEC");
        const int sC = ReadGlobalNumber("MIN_TIME_MIL");  // centiseconds
        if (sH < 0 || sM < 0 || sS < 0 || sC < 0) return;
        const uint32_t startMs = static_cast<uint32_t>(
            (((sH * static_cast<int>(mph) + sM) * 60 + sS) * 1000) + sC * 10);
        const long initialElapsed =
            static_cast<long>(nowMs) - static_cast<long>(startMs);
        if (initialElapsed < 0 || initialElapsed > kRaceStartFreshMaxMs) return;
        g_state.race_start_ms = startMs;
        g_state.race_mph      = mph;
        g_state.have_start_ms = true;
        acclog::Write("SwoopRace",
                      "race timer start: %d:%d:%d.%02d mph=%u startMs=%u "
                      "nowMs=%u initElapsed=%ldms",
                      sH, sM, sS, sC, mph, startMs, nowMs, initialElapsed);
    }

    if (maxSpeed > g_state.race_max_speed) g_state.race_max_speed = maxSpeed;
    if (speed    > g_state.race_peak_speed) g_state.race_peak_speed = speed;

    // Elapsed, every active tick. No speed-band gate: the finish detector
    // above freezes this at the crossing, so a slow run that never reaches
    // top gear still counts its full duration to the finish line.
    uint32_t nowMs = 0, mph = g_state.race_mph;
    if (!ReadWorldClockMs(server, nowMs, mph)) return;
    const long elapsedMs =
        static_cast<long>(nowMs) - static_cast<long>(g_state.race_start_ms);
    if (elapsedMs < 0) return;                       // clock wrap — skip tick
    const float seconds = elapsedMs / 1000.0f;
    if (seconds > kRaceTimeMaxSeconds) return;       // implausible — reject
    g_state.race_time_seconds = seconds;
    g_state.have_race_time    = true;
}

void SpeakRaceEndMessage() {
    if (g_state.race_time_announced) return;  // idempotent — first call wins
    g_state.race_time_announced = true;

    char buf[160];
    const char* msg;
    if (g_state.wrecked) {
        // A wreck is not a finish: no time, say what happened.
        msg = acc::strings::Get(acc::strings::Id::SwoopRaceWrecked);
    } else if (g_state.have_race_time) {
        // The time-bearing variant leads with the number and includes the
        // "race ended" phrase.
        int whole = static_cast<int>(g_state.race_time_seconds);
        int centi = static_cast<int>(
            (g_state.race_time_seconds - static_cast<float>(whole)) * 100.0f
            + 0.5f);
        if (centi >= 100) { centi = 0; ++whole; }
        std::snprintf(buf, sizeof(buf),
                      acc::strings::Get(acc::strings::Id::FmtSwoopRaceTime),
                      whole, centi);
        msg = buf;
    } else {
        msg = acc::strings::Get(acc::strings::Id::SwoopRaceEnded);
    }
    if (g_state.wrecked || !g_state.have_race_time) {
        acc::speech::Speak(msg, /*interrupt=*/true);
    } else {
        // The time waits out the finish sound (see kRaceEndSpeakDelayMs);
        // FlushRaceEndSpeech speaks it from the tick, or at once on exit.
        std::snprintf(g_state.race_end_msg, sizeof(g_state.race_end_msg), "%s", msg);
        g_state.race_end_pending = true;
        g_state.race_end_due_ms  = GetTickCount64() + kRaceEndSpeakDelayMs;
    }
    acclog::Write("SwoopRace",
                  "spoke race end: [%s] (raceTime=%.2f have=%d%s)",
                  msg, g_state.race_time_seconds,
                  g_state.have_race_time ? 1 : 0,
                  g_state.race_end_pending ? " delayed" : "");
}

// Speaks the pending race time once its delay has elapsed (force = the race
// module is exiting: speak whatever is pending now rather than lose it).
void FlushRaceEndSpeech(bool force) {
    if (!g_state.race_end_pending) return;
    if (!force && GetTickCount64() < g_state.race_end_due_ms) return;
    g_state.race_end_pending = false;
    acc::speech::Speak(g_state.race_end_msg, /*interrupt=*/true);
    acclog::Write("SwoopRace", "spoke race time%s: [%s]",
                  force ? " (flushed on exit)" : "", g_state.race_end_msg);
}

}  // namespace acc::swoop_race

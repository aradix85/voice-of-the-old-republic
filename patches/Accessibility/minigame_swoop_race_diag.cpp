// The swoop race's diagnostic log lines: the minigame at entry and a periodic
// snapshot of the bike.

#include "minigame_swoop_race_internal.h"

#include "engine_area.h"
#include "engine_game.h"
#include "log.h"

namespace acc::swoop_race {

namespace {

// Snapshot cadence: 1 Hz keeps the log usable and still shows the speed
// curve.
constexpr ULONGLONG kDiagLogIntervalMs    = 1000;

}  // namespace

void EmitEntrySummary(void* miniGame) {
    uint32_t type      = ReadU32Or0(miniGame, kMiniGameTypeOffset);
    uint32_t enemies   = ReadU32Or0(miniGame, kMiniGameEnemyCountOffset);
    uint32_t obstacles = ReadU32Or0(miniGame, kMiniGameObstacleCountOffset);
    void*    obstData  = ReadPtrOrNull(miniGame, kMiniGameObstacleDataOffset);
    void*    player    = ReadPtrOrNull(miniGame, kMiniGamePlayerOffset);
    acclog::Write("SwoopRace",
                  "ENTER mini_game=%p vtable=%p type=%u enemies=%u "
                  "obstacles=%u obstacles_ptr=%p player=%p",
                  miniGame, g_state.latch.vtable, type, enemies, obstacles,
                  obstData, player);
}

void EmitDiagSnapshot(void* miniGame) {
    ULONGLONG now = GetTickCount64();
    if (now - g_state.last_diag_log_ms < kDiagLogIntervalMs) return;
    g_state.last_diag_log_ms = now;

    void* player = ReadPtrOrNull(miniGame, kMiniGamePlayerOffset);
    if (!player) {
        acclog::Write("SwoopRace", "snapshot: player ptr null");
        return;
    }
    float speed    = ReadFloatOr0(player, kFollowerSpeedOffset);
    float minSpeed = ReadFloatOr0(player, kMiniPlayerMinSpeedOffset);
    float maxSpeed = ReadFloatOr0(player, kMiniPlayerMaxSpeedOffset);
    Vector off;
    const bool offOk = TryReadVector(player, kMiniPlayerOffsetVectorOffset, &off);
    // The scripts' gear global and (K2) the jump velocity ride along so a log
    // alone shows whether the bike is in the countdown (<0), racing (0..5) or
    // finished (6), and whether a Space press actually launched a hop.
    const int   scriptGear = acc::engine::ReadGlobalNumber(kGearGlobalName);
    const float jumpV      = acc::game::IsKotor2()
        ? ReadFloatOr0(miniGame, kMiniGameJumpVelocityOffset) : 0.0f;
    acclog::Write("SwoopRace",
                  "snapshot speed=%.2f [%.2f..%.2f] gear=%d global=%d jumpV=%.1f "
                  "tunnel=%s (%.2f,%.2f,%.2f)",
                  speed, minSpeed, maxSpeed, g_state.gear, scriptGear, jumpV,
                  offOk ? "ok" : "FAULT", off.x, off.y, off.z);
}

}  // namespace acc::swoop_race

// Obstacle cues of the swoop race: which objects are hazards (scenery,
// mines, bars, posts), their boxes and the gap in a bar, mines passed, and
// the per-tick obstacle cues.

#include "minigame_swoop_audio_internal.h"

#include "audio_bus.h"
#include "engine_area.h"
#include "engine_game.h"
#include "engine_player.h"    // listener: the camera; the engine swaps the
                              //     player creature during the race
#include "log.h"

namespace acc::swoop_race {

namespace {

// ============================================================================
// Engine struct offsets used by the spatial walks.
// ============================================================================
// CSWMGObstacle (line ~17456 in re/swkotor.exe.h) — AurObject pointer
// at +0x60, then world position via the AurObject's Gob at +0x78.
//
// KOTOR 2: the AurObject pointer is UNMOVED (its ctor twin @0x00838ad0 nulls
// dword index 0x18 = +0x60 right after the CSWMiniGameObject base, and its
// GetName twin @0x00838c80 reads [this+0x60] then that object's vtable[+0xc] —
// the same two hops as KOTOR 1's, so kAurVtableSlotGetName is unchanged too).
// CSWMiniGameObject itself is byte-identical in both games (size 0x60).
//
// The POSITION inside the Gob did move, 0x78 -> 0xa4. Witnessed on both sides
// through the same door: Gob's vtable slot +0x64 is the position getter, and
// its body is a bare "copy three floats from this+N". KOTOR 1's (0x00449980)
// does `add ecx,0x78`; KOTOR 2's (0x00459710) does `add eax,0xa4`. That slot is
// the one CSWTrackFollower::GetPosition calls on a model in both games, which is
// what ties the two witnesses to the same field.
const size_t kObstacleAurObjectOffset       = acc::off::Same(0x60);
const size_t kAurObjectPositionOffset       = acc::off::Pick(0x78, 0xa4);

// Cap on how many obstacle loops may sound at once, nearest first.
//
// KOTOR 1 never needed one: its swoop areas carry 22 obstacles over a whole
// track, so the 300 m window holds a handful. KOTOR 2's carry 151 (211TEL), 62
// (371NAR) and 65 (510OND) — counted from each area's own Obstacles list — and
// an uncapped sweep there would start dozens of concurrent loops of the same
// metallic sample, which is both a wall of noise and more 3D voices than the
// mixer has. The cap is per-game rather than shared precisely because the
// justification is a measured content difference; KOTOR 1's sweep stays
// bit-identical.
//
// 6 is the working figure for the first K2 round: enough to hear a cluster as a
// cluster, few enough to keep them separable. Re-tune from the round's
// "inRange" counts, which the scan line already logs.
int ObstacleLoopBudget() {
    return acc::game::IsKotor2() ? 6 : kMgoArraySlotCount;
}

// The `<track>_MGE*` scenery objects (see the constants block): the token
// "_MGE" anywhere in the resref, case-insensitive. Same hand-rolled match as
// the mine test in TickAccelpadCues — no <cctype> / locale in SEH-heavy code.
bool IsSceneryObstacleName(const char* name) {
    if (!name) return false;
    for (const char* p = name; *p; ++p) {
        if (p[0] == '_' && (p[1] == 'm' || p[1] == 'M') &&
            (p[2] == 'g' || p[2] == 'G') && (p[3] == 'e' || p[3] == 'E')) {
            return true;
        }
    }
    return false;
}

// Resolve and classify one obstacle's collision box (KOTOR 2). Fills `hz`
// once; later ticks reuse it. Falls back to "failed" (= position-based cue,
// the K1 behaviour) when neither the part nor the gob box reads sanely.
void ResolveHazardBox(void* aur, const char* name, HazardInfo& hz) {
    Vector mn = {0, 0, 0}, mx = {0, 0, 0};
    bool ok = ReadGobPartBoundingBox(aur, "hitbump", mn, mx);
    hz.from_part = ok ? 1 : 0;
    if (!ok) ok = ReadGobBoundingBox(aur, mn, mx);
    if (ok) {
        // Sanity: a whole-gob box that swallows a shuttle model or a broken
        // read is not a hazard box. Real collision meshes here are < 60 units
        // in every axis (widest: a full-lane bar, 40 units).
        const float dx = mx.x - mn.x, dy = mx.y - mn.y, dz = mx.z - mn.z;
        if (!(dx >= 0.0f && dy >= 0.0f && dz >= 0.0f) ||
            dx > 120.0f || dy > 120.0f || dz > 120.0f) {
            ok = false;
        }
    }
    if (!ok) { hz.state = kHazardFailed; return; }
    hz.state = kHazardOk;
    hz.mn = mn; hz.mx = mx;
    if (IsSceneryObstacleName(name)) { hz.cls = kHazardScenery; return; }
    hz.cls = ((mx.z - mn.z) <= kJumpableHeightU) ? kHazardBar : kHazardBlock;
}

}  // namespace

namespace {

// Bike state for the K2 bar logic (lane, forward position, speed, the
// airborne edge for the "jump again" repeat) and the listener; false without
// a listener anchor.
bool ReadObstacleTick(void* miniGame, ObstacleTick& t) {
    t.k2 = acc::game::IsKotor2();
    void* player = SafeReadPtr(miniGame, kMiniGamePlayerOffset);
    t.bikeOk = ReadFollowerPosition(player, t.bikePos);
    t.bikeSpeed = SafeReadFloat(player, kFollowerSpeedOffset);
    t.playerRadius = SafeReadFloat(player, kFollowerSphereRadiusOffset);
    Vector tunnel = {0, 0, 0};
    const bool tunOk = SafeReadVector(player, kMiniPlayerOffsetVectorOffset, tunnel);
    const bool airborne = tunOk && tunnel.z > kAirborneZ;
    t.justLanded = g_state.was_airborne && !airborne;
    g_state.was_airborne = airborne;
    // Parked (gear 0 after a hit): every bar's beep re-arms, so each relaunch
    // in front of a bar gets the beep again, on the free side. Round 9 lost
    // seven hits in five seconds relaunching into MGC03 with no cue at all
    // (the two beeps per pass had been spent before the hit).
    t.parked = t.k2 && acc::engine::ReadGlobalNumber("MIN_RACE_GEAR") == 0;

    // During the race the engine swaps the controlled creature (PartyLeader
    // goes handle=0xffffffff), so GetPlayerPosition is unreliable; the camera
    // follows the bike and is the right listener.
    if (!acc::engine::GetCameraPosition(t.listener) &&
        !acc::engine::GetPlayerPosition(t.listener)) {
        acclog::Trace("SwoopRace", "obstacle cues: no listener anchor available");
        return false;
    }
    return true;
}

bool ReadObstacle(void* mgoArray, int i, ObstacleTick& t, ObstacleView& v) {
    void* obj =
        SafeReadPtr(mgoArray, kMgoArrayObjectsOffset + static_cast<size_t>(i) * sizeof(void*));
    if (!obj) return false;
    ++t.slotsSeen;
    // AsObstacle (vtable[8]) returns the object itself for a CSWMGObstacle,
    // null otherwise: the engine's own type check.
    v.obstacle = CallAsCast(obj, kVtableSlotAsObstacle);
    if (!v.obstacle) return false;
    ++t.obstaclesFound;
    void* aur = SafeReadPtr(v.obstacle, kObstacleAurObjectOffset);
    if (!aur || !SafeReadVector(aur, kAurObjectPositionOffset, v.pos)) return false;
    // For the inventory log and the K2 classification (rocks and accelpads
    // are in different engine pools).
    v.name = ReadAurObjectName(aur);

    HazardInfo& hz = g_state.hazards[i];
    if (t.k2 && hz.state == kHazardUntried) ResolveHazardBox(aur, v.name, hz);
    v.haveBox = t.k2 && hz.state == kHazardOk;
    v.x0 = v.haveBox ? v.pos.x + hz.mn.x : v.pos.x;
    v.x1 = v.haveBox ? v.pos.x + hz.mx.x : v.pos.x;
    v.y0 = v.haveBox ? v.pos.y + hz.mn.y : v.pos.y;
    v.y1 = v.haveBox ? v.pos.y + hz.mx.y : v.pos.y;
    v.z0 = v.haveBox ? v.pos.z + hz.mn.z : v.pos.z;
    v.scenery = v.haveBox && hz.cls == kHazardScenery;
    v.post = v.haveBox && !v.scenery && (v.x0 >= kLaneEdgeSkipU || v.x1 <= -kLaneEdgeSkipU);
    v.hazardPos = v.pos;
    if (v.haveBox) {
        float cx = 0.5f * (v.x0 + v.x1);
        if (cx >  kLaneHalfWidthU) cx =  kLaneHalfWidthU;
        if (cx < -kLaneHalfWidthU) cx = -kLaneHalfWidthU;
        v.hazardPos.x = cx;
        v.hazardPos.y = v.y0;
        v.hazardPos.z = v.z0;
    }
    return true;
}

// First sweep only: every obstacle's slot and name (and the K2 box and
// class), so the per-track inventory is in the log once.
void LogObstacleInventory(int i, const ObstacleView& v, const HazardInfo& hz, bool k2) {
    void* vt = SafeReadPtr(v.obstacle, 0);
    if (v.haveBox) {
        acclog::Write("SwoopRace",
                      "obstacle slot=%d ptr=%p vtable=%p name=[%s] "
                      "pos=(%.2f,%.2f,%.2f) box=%s x[%.1f..%.1f] "
                      "y[%.1f..%.1f] z[%.1f..%.1f] class=%s",
                      i, v.obstacle, vt, v.name ? v.name : "(null)",
                      v.pos.x, v.pos.y, v.pos.z,
                      hz.from_part ? "hitbump" : "gob",
                      v.x0, v.x1, v.y0, v.y1, v.z0, v.pos.z + hz.mx.z,
                      v.scenery ? "scenery" : v.post ? "post"
                      : (hz.cls == kHazardBar ? "bar" : "block"));
    } else {
        acclog::Write("SwoopRace",
                      "obstacle slot=%d ptr=%p vtable=%p name=[%s] "
                      "pos=(%.2f,%.2f,%.2f)%s",
                      i, v.obstacle, vt, v.name ? v.name : "(null)",
                      v.pos.x, v.pos.y, v.pos.z,
                      (k2 && hz.state == kHazardFailed) ? " box=FAILED" : "");
    }
}

// A hazard that warns through the loop (K1 rocks, K2 blocks and mines):
// ahead of the listener within the margin, and inside range. Track Y is the
// forward axis (low to high), confirmed live on K1 and asserted per race on
// K2 by the axis probe.
bool Admit(ObstacleTick& t, int slot, const Vector& hazardPos, const char* name) {
    if (hazardPos.y < t.listener.y - kObstacleForwardMargin) return false;
    ++t.ahead;
    const float distSq = DistXYZSq(hazardPos, t.listener);
    if (distSq > kObstacleCueRangeM * kObstacleCueRangeM) return false;
    ++t.inRange;
    ObstacleTick::Candidate& c = t.candidates[t.candidateCount++];
    c.slot = slot;
    c.dist = (distSq > 0.0f) ? std::sqrt(distSq) : 0.0f;
    c.pos  = hazardPos;
    c.name = name;
    return true;
}

// Nearest first, cut to the loop budget: a partial selection sort (single
// digits on K2; on K1 the budget is the whole array). Returns how many sound.
int SelectNearest(ObstacleTick& t) {
    const int sounding = (t.candidateCount < ObstacleLoopBudget()) ? t.candidateCount
                                                                    : ObstacleLoopBudget();
    for (int a = 0; a < sounding; ++a) {
        int best = a;
        for (int b = a + 1; b < t.candidateCount; ++b) {
            if (t.candidates[b].dist < t.candidates[best].dist) best = b;
        }
        if (best != a) {
            const ObstacleTick::Candidate tmp = t.candidates[a];
            t.candidates[a] = t.candidates[best];
            t.candidates[best] = tmp;
        }
    }
    return sounding;
}

// The decoupled cue: lateral and vertical true, forward pinned to a fixed
// reference depth, so the pan says lane error only; loudness follows our
// own curve on the real 3D distance. Returns the loops started.
int SoundWarnLoops(const ObstacleTick& t, int sounding, bool active[kMgoArraySlotCount]) {
    int started = 0;
    for (int c = 0; c < sounding; ++c) {
        const ObstacleTick::Candidate& cand = t.candidates[c];
        const int i = cand.slot;
        Vector cuePos = cand.pos;
        cuePos.y = t.listener.y + kSwoopPanForwardRefM;
        const int volByte = SwoopVolumeByte(cand.dist, kObstacleCueRangeM);
        active[i] = true;
        if (g_state.obstacle_loops[i].IsActive()) {
            g_state.obstacle_loops[i].UpdatePosition(cuePos);
            g_state.obstacle_loops[i].UpdateVolume(volByte);
            continue;
        }
        // The flat band, so only our own volume shapes the loudness.
        if (g_state.obstacle_loops[i].Start(
                kObstacleWarnLoopResref, cuePos, /*looping=*/true, /*spatial=*/true,
                /*priorityGroup=*/-1, /*volumeByte=*/volByte,
                /*maxVolDist=*/kSwoopFlatBandMaxM,
                /*minVolDist=*/kSwoopFlatBandMinM)) {
            ++started;
            acclog::Trace("SwoopRace",
                          "loop start slot=%d name=[%s] "
                          "obstaclePos=(%.1f,%.1f,%.1f) "
                          "cuePos=(%.1f,%.1f,%.1f) dist=%.1f res=%s",
                          i, cand.name ? cand.name : "(null)",
                          cand.pos.x, cand.pos.y, cand.pos.z,
                          cuePos.x, cuePos.y, cuePos.z,
                          cand.dist, kObstacleWarnLoopResref);
        }
    }
    return started;
}

// Loops whose obstacle left range (passed, or its reads failed this tick);
// Stop is idempotent. Returns the loops stopped.
int StopStaleLoops(const bool active[kMgoArraySlotCount]) {
    int stopped = 0;
    for (int i = 0; i < kMgoArraySlotCount; ++i) {
        if (active[i] || !g_state.obstacle_loops[i].IsActive()) continue;
        g_state.obstacle_loops[i].Stop();
        ++stopped;
        acclog::Trace("SwoopRace", "loop stop slot=%d (out of range / unresolved)", i);
    }
    return stopped;
}

}  // namespace

void TickObstacleCues(void* miniGame) {
    void* mgoArray = ResolveMgoArray();
    if (!mgoArray) {
        acclog::Trace("SwoopRace", "obstacle cues: mgo array unresolved");
        return;
    }
    ObstacleTick t;
    if (!ReadObstacleTick(miniGame, t)) return;
    g_state.parked_bar = ParkedBar::None;   // re-classified by the bar step each tick

    for (int i = 0; i < kMgoArraySlotCount; ++i) {
        ObstacleView v;
        if (!ReadObstacle(mgoArray, i, t, v)) continue;
        HazardInfo& hz = g_state.hazards[i];
        if (!g_state.obstacle_diag_emitted) LogObstacleInventory(i, v, hz, t.k2);
        if (v.post)    { ++t.postsSkipped;   continue; }  // the wall cue covers these
        if (v.scenery) { ++t.scenerySkipped; continue; }  // phantom boxes, never hit
        if (v.haveBox && hz.cls == kHazardBar) {
            HandleBar(i, v, hz, t);
            continue;
        }
        Admit(t, i, v.hazardPos, v.name);
    }
    for (int b = 0; b < t.newBarCount; ++b) g_state.bars[b] = t.newBars[b];
    g_state.bar_count = t.newBarCount;

    // K2 mines, collected by this tick's pad sweep, warn like blocks.
    for (int m = 0; m < g_state.mine_count; ++m) {
        if (Admit(t, g_state.mine_slot[m], g_state.mine_pos[m], "mine")) ++t.minesInRange;
    }

    const int sounding = SelectNearest(t);
    bool active[kMgoArraySlotCount] = {};
    const int started = SoundWarnLoops(t, sounding, active);
    const int stopped = StopStaleLoops(active);

    // Only after the whole sweep, so the first one logs every obstacle.
    if (!g_state.obstacle_diag_emitted && t.obstaclesFound > 0) {
        g_state.obstacle_diag_emitted = true;
    }
    if (!g_state.bar_gap_diag_emitted && g_state.mine_count > 0 && t.newBarCount > 0) {
        g_state.bar_gap_diag_emitted = true;
    }
    if (t.k2) {
        acclog::Trace("SwoopRace",
                      "hazards: posts=%d scenery=%d bars=%d mines=%d/%d inRange",
                      t.postsSkipped, t.scenerySkipped, t.barsSeen, t.minesInRange,
                      g_state.mine_count);
    }
    acclog::Trace("SwoopRace",
                  "scan: slots=%d obstacles=%d ahead=%d inRange=%d (%.0fm) "
                  "overBudget=%d started=%d stopped=%d listenerY=%.1f",
                  t.slotsSeen, t.obstaclesFound, t.ahead,
                  t.inRange, kObstacleCueRangeM, t.candidateCount - sounding,
                  started, stopped, t.listener.y);
}

}  // namespace acc::swoop_race

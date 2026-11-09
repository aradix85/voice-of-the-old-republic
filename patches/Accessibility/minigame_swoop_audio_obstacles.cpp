// Obstacle cues of the swoop race: which objects are hazards (scenery,
// mines, bars, posts), their boxes and the gap in a bar, mines passed, and
// the per-tick obstacle cues.

#include "minigame_swoop_audio_internal.h"

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

// ============================================================================
// Continuous obstacle-proximity cue parameters.
// ============================================================================
//
//   kObstacleCueRangeM     — only obstacles within this 3D distance from
//                            the listener fire cues. Sized for ~1-1.3 s
//                            of advance warning at the top observed bike
//                            speed (~190 units/s in gear 3 on Taris), so
//                            the user has time to choose a lane before
//                            the obstacle arrives. Earlier 30 m was sized
//                            like the in-world Pillar-1 walls and gave
//                            only ~150 ms warning — confirmed too late by
//                            the user 2026-05-24.
//
//   kObstacleWarnLoopResref — engine sample looped for each obstacle.
//                            cb_gr_boncehard2 — a short metallic grenade-bounce
//                            SFX with steady pitch/volume, so looping it reads as
//                            a constant metallic clicking that can't be ignored
//                            in the ~0.7s in-range window. Replaced mgs_hover_07l,
//                            a quiet sustained hover drone whose faint attack was
//                            lost in that window (the "obstacles easy to ignore"
//                            bug — the same long-drone problem the accelpad loop
//                            sample was trimmed to fix).
//
//   kObstacleForwardMargin — obstacles whose Y (forward axis on the
//                            tunnel track — the bike moves from low Y to
//                            high Y, confirmed live) are dropped if they
//                            sit behind the listener by more than this
//                            margin. Small negative margin allows
//                            immediately-adjacent obstacles (mid-pass) to
//                            keep cueing for one tick so the spatial pan
//                            completes its sweep.
constexpr float       kObstacleCueRangeM     = 300.0f;
constexpr float       kObstacleForwardMargin = 10.0f;

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

// ============================================================================
// KOTOR 2 half-bar free-side strips (kPostContactU block) + the mine-pass log.
// ============================================================================

// Lateral reach of mine `m` at the bike's ground height: sqrt(R² − dz²) plus
// slack, R = the measured combined reach (kMineCollisionRadiusU — the live
// sphere_radius is NOT the collision radius, see there).
float MineLateralReach(int m, float bikeZ, float /*playerRadius*/) {
    const float R  = kMineCollisionRadiusU;
    const float dz = g_state.mine_pos[m].z - bikeZ;
    const float sq = R * R - dz * dz;
    return (sq > 0.0f ? std::sqrt(sq) : 0.0f) + kBarStripSlackU;
}

// One side's strip [lo, hi] minus every mine near the bar. Returns the width
// of the widest surviving piece (0 = none) and its bounds.
float WidestStripPiece(float lo, float hi, float barY, float bikeZ, float playerRadius,
                       float& outLo, float& outHi) {
    float pl[8], ph[8]; int n = 0;
    if (lo < hi) { pl[0] = lo; ph[0] = hi; n = 1; }
    for (int m = 0; m < g_state.mine_count && n > 0; ++m) {
        if (std::fabs(g_state.mine_pos[m].y - barY) > kBarMineWindowU) continue;
        const float reach = MineLateralReach(m, bikeZ, playerRadius);
        const float mlo = g_state.mine_pos[m].x - reach;
        const float mhi = g_state.mine_pos[m].x + reach;
        float nl[8], nh[8]; int k = 0;
        for (int i = 0; i < n; ++i) {
            if (mhi <= pl[i] || mlo >= ph[i]) {            // untouched piece
                if (k < 8) { nl[k] = pl[i]; nh[k] = ph[i]; ++k; }
                continue;
            }
            if (mlo > pl[i] && k < 8) { nl[k] = pl[i]; nh[k] = mlo;   ++k; }
            if (mhi < ph[i] && k < 8) { nl[k] = mhi;   nh[k] = ph[i]; ++k; }
        }
        for (int i = 0; i < k; ++i) { pl[i] = nl[i]; ph[i] = nh[i]; }
        n = k;
    }
    float best = 0.0f;
    for (int i = 0; i < n; ++i) {
        const float w = ph[i] - pl[i];
        if (w > best) { best = w; outLo = pl[i]; outHi = ph[i]; }
    }
    return best;
}

// Fills bar.has_gap / gap_x / gap_side (-1 left strip, +1 right strip).
void ComputeBarGap(SpatialAudioState::BarInfo& bar, float bikeZ, float playerRadius) {
    float lLo = 0.0f, lHi = 0.0f, rLo = 0.0f, rHi = 0.0f;
    const float lw = WidestStripPiece(-kPostContactU + kBarStripSlackU, bar.x0 - kBarFreeClearU,
                                      bar.y0, bikeZ, playerRadius, lLo, lHi);
    const float rw = WidestStripPiece(bar.x1 + kBarFreeClearU, kPostContactU - kBarStripSlackU,
                                      bar.y0, bikeZ, playerRadius, rLo, rHi);
    bar.has_gap = false; bar.gap_x = 0.0f; bar.gap_side = 0;
    const bool lOk = lw >= kBarStripMinU, rOk = rw >= kBarStripMinU;
    if (!lOk && !rOk) return;
    const bool useLeft = lOk && (!rOk || lw >= rw);
    bar.has_gap  = true;
    bar.gap_side = useLeft ? -1 : +1;
    if (useLeft) bar.gap_x = (lw >= kBarStripWideU) ? lHi : 0.5f * (lLo + lHi);
    else         bar.gap_x = (rw >= kBarStripWideU) ? rLo : 0.5f * (rLo + rHi);
}

}  // namespace

// Mine-pass log: per mine, the closest 3-D approach while the mine is within
// kMinePassWindowU ahead or behind, logged once when it falls behind, with
// the hp before and after. Hits at a 2-D distance inside R but a 3-D one
// outside it would say the collision is flat; clean passes the other way
// round say it is spherical (the model kPostContactU's block assumes).
void TrackMinePasses(void* player, const Vector& bikePos, float playerRadius) {
    const int hp = static_cast<int>(SafeReadU32(player, kFollowerHpOffset));
    for (int m = 0; m < g_state.mine_count; ++m) {
        const int slot = g_state.mine_slot[m];
        if (slot < 0 || slot >= kMgoArraySlotCount) continue;
        const Vector& mp = g_state.mine_pos[m];
        const float dx = mp.x - bikePos.x, dy = mp.y - bikePos.y, dz = mp.z - bikePos.z;
        if (dy > kMinePassWindowU) continue;                     // still far ahead
        if (dy < -kMinePassWindowU) {
            if (!g_state.mine_pass_open[slot]) continue;
            g_state.mine_pass_open[slot] = false;
            const float pdx = g_state.mine_pass_dx[slot], pdy = g_state.mine_pass_dy[slot],
                        pdz = g_state.mine_pass_dz[slot];
            const int hp0 = g_state.mine_pass_hp0[slot];
            acclog::Write("SwoopRace",
                          "mine pass: slot=%d mine=(%.1f,%.1f,%.1f) r=%.2f bikeR=%.2f bikeZ=%.2f "
                          "minD3=%.2f at dx=%.2f dy=%.2f dz=%.2f d2=%.2f hp %d->%d hit=%d",
                          slot, mp.x, mp.y, mp.z, g_state.mine_radius[m], playerRadius,
                          bikePos.z, g_state.mine_pass_min_d3[slot], pdx, pdy, pdz,
                          std::sqrt(pdx * pdx + pdy * pdy), hp0, hp,
                          (hp0 > 0 && hp < hp0) ? 1 : 0);
            continue;
        }
        const float d3 = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (!g_state.mine_pass_open[slot]) {
            g_state.mine_pass_open[slot]   = true;
            g_state.mine_pass_min_d3[slot] = d3;
            g_state.mine_pass_dx[slot] = dx; g_state.mine_pass_dy[slot] = dy; g_state.mine_pass_dz[slot] = dz;
            g_state.mine_pass_hp0[slot]    = hp;
        } else if (d3 < g_state.mine_pass_min_d3[slot]) {
            g_state.mine_pass_min_d3[slot] = d3;
            g_state.mine_pass_dx[slot] = dx; g_state.mine_pass_dy[slot] = dy; g_state.mine_pass_dz[slot] = dz;
        }
    }
}

void TickObstacleCues(void* miniGame) {
    void* mgoArray = ResolveMgoArray();
    if (!mgoArray) {
        acclog::Trace("SwoopRace", "obstacle cues: mgo array unresolved");
        return;
    }

    // Bike state for the K2 bar logic: lane position, forward position, speed,
    // and the airborne edge (offset.z) for the "jump again" repeat.
    const bool k2 = acc::game::IsKotor2();
    void*  player   = SafeReadPtr(miniGame, kMiniGamePlayerOffset);
    Vector bikePos  = {0, 0, 0};
    const bool bikeOk = ReadFollowerPosition(player, bikePos);
    const float bikeSpeed = SafeReadFloat(player, kFollowerSpeedOffset);
    const float playerRadius = SafeReadFloat(player, kFollowerSphereRadiusOffset);
    Vector tunnel = {0, 0, 0};
    const bool tunOk = SafeReadVector(player, kMiniPlayerOffsetVectorOffset, tunnel);
    const bool airborne = tunOk && tunnel.z > kAirborneZ;
    const bool justLanded = g_state.was_airborne && !airborne;
    g_state.was_airborne = airborne;
    // Parked (gear 0 after a hit): every bar's beep re-arms, so each relaunch
    // in front of a bar gets the beep again, on the free side. Round 9 lost
    // seven hits in five seconds relaunching into MGC03 with no cue at all
    // (the two beeps per pass had been spent before the hit).
    const bool parked = k2 && acc::engine::ReadGlobalNumber("MIN_RACE_GEAR") == 0;

    // Listener anchor. During the race the engine swaps the controlled
    // creature (PartyLeader log goes handle=0xffffffff), so
    // GetPlayerPosition is unreliable. Camera follows the bike, so
    // it's the correct listener.
    Vector listener_pos;
    if (!acc::engine::GetCameraPosition(listener_pos) &&
        !acc::engine::GetPlayerPosition(listener_pos)) {
        acclog::Trace("SwoopRace",
                      "obstacle cues: no listener anchor available");
        return;
    }

    int slots_seen = 0;
    int obstacles_found = 0;
    int obstacles_ahead = 0;
    int obstacles_in_range = 0;
    int obstacles_dropped = 0;   // in range but over the concurrency budget
    int loops_started_this_tick = 0;
    int loops_stopped_this_tick = 0;

    // In-range obstacles collected this tick, before the nearest-N cut. POD, so
    // no destructors — this file is SEH-heavy by convention (see CLAUDE.md).
    struct ObstacleCandidate {
        int         slot;
        float       dist;
        Vector      pos;
        const char* name;
    };
    ObstacleCandidate candidates[kMgoArraySlotCount];
    int candidate_count = 0;

    // Shared admission test for a hazard that warns through the warn loop
    // (K1 rocks, K2 blocks, K2 mines): ahead of the listener (within the
    // margin), inside range. Appends to `candidates`; returns false if not.
    auto admit = [&](int slot, const Vector& hazardPos, const char* name,
                     int& aheadCount, int& inRangeCount) -> bool {
        if (hazardPos.y < listener_pos.y - kObstacleForwardMargin) return false;
        ++aheadCount;
        const float distSq = DistXYZSq(hazardPos, listener_pos);
        if (distSq > kObstacleCueRangeM * kObstacleCueRangeM) return false;
        ++inRangeCount;
        candidates[candidate_count].slot = slot;
        candidates[candidate_count].dist = (distSq > 0.0f) ? std::sqrt(distSq) : 0.0f;
        candidates[candidate_count].pos  = hazardPos;
        candidates[candidate_count].name = name;
        ++candidate_count;
        return true;
    };
    int posts_skipped = 0, scenery_skipped = 0, bars_seen = 0, mines_in_range = 0;

    // Per-tick "still in range" flag. Slots flipped true here are
    // either Started (loop wasn't active) or just UpdatePosition'd
    // (loop already running). At end of tick, any slot whose loop is
    // active but flag is false has gone out of range and gets Stop'd.
    bool active_this_tick[kMgoArraySlotCount] = {};

    // This tick's bars (K2), published to g_state after the sweep so the
    // pair lookup inside it reads a consistent (last tick's) list.
    SpatialAudioState::BarInfo newBars[kMgoArraySlotCount];
    int newBarCount = 0;
    g_state.parked_bar = ParkedBar::None;   // re-classified by the bar branch each tick

    for (int i = 0; i < kMgoArraySlotCount; ++i) {
        // mgoArray->objects[i]
        void* obj = SafeReadPtr(mgoArray,
                                kMgoArrayObjectsOffset +
                                static_cast<size_t>(i) * sizeof(void*));
        if (!obj) continue;
        ++slots_seen;

        // AsObstacle vtable[8] returns the same pointer if `obj` is a
        // CSWMGObstacle, null otherwise. That's the engine's own
        // type-check pattern.
        void* obstacle = CallAsCast(obj, kVtableSlotAsObstacle);
        if (!obstacle) continue;
        ++obstacles_found;

        // Position via CSWMGObstacle +0x60 -> CAurObject +0x78.
        void* aur = SafeReadPtr(obstacle, kObstacleAurObjectOffset);
        if (!aur) continue;
        Vector pos;
        if (!SafeReadVector(aur, kAurObjectPositionOffset, pos)) continue;

        // Read the obstacle's name via CAurObject's GetName vtable
        // accessor. Used for the first-fire inventory log; cue routing
        // is no longer name-based (rocks and accelpads are in different
        // engine pools — see the "Object pool split" comment at top).
        const char* name = ReadAurObjectName(aur);

        // ---- KOTOR 2: classify by collision box (see the constants block). ----
        HazardInfo& hz = g_state.hazards[i];
        if (k2 && hz.state == kHazardUntried) ResolveHazardBox(aur, name, hz);
        const bool haveBox = k2 && hz.state == kHazardOk;
        // World box (origin + local; these objects are unrotated on straight
        // +Y tracks — the axis probe has already asserted +Y).
        const float wx0 = haveBox ? pos.x + hz.mn.x : pos.x;
        const float wx1 = haveBox ? pos.x + hz.mx.x : pos.x;
        const float wy0 = haveBox ? pos.y + hz.mn.y : pos.y;
        const float wy1 = haveBox ? pos.y + hz.mx.y : pos.y;
        const float wz0 = haveBox ? pos.z + hz.mn.z : pos.z;
        const bool  isScenery = haveBox && hz.cls == kHazardScenery;
        const bool  isPost = haveBox && !isScenery &&
                             (wx0 >= kLaneEdgeSkipU || wx1 <= -kLaneEdgeSkipU);
        // Where the hazard really is: box centre laterally (clamped to the
        // lane), leading edge forward, base height.
        Vector hazardPos = pos;
        if (haveBox) {
            float cx = 0.5f * (wx0 + wx1);
            if (cx >  kLaneHalfWidthU) cx =  kLaneHalfWidthU;
            if (cx < -kLaneHalfWidthU) cx = -kLaneHalfWidthU;
            hazardPos.x = cx; hazardPos.y = wy0; hazardPos.z = wz0;
        }

        // First-fire diagnostic: log EVERY obstacle's slot + name (+ the K2
        // box and class) so the per-track inventory is in the log once.
        if (!g_state.obstacle_diag_emitted) {
            void* vt = SafeReadPtr(obstacle, 0);
            if (haveBox) {
                acclog::Write("SwoopRace",
                              "obstacle slot=%d ptr=%p vtable=%p name=[%s] "
                              "pos=(%.2f,%.2f,%.2f) box=%s x[%.1f..%.1f] "
                              "y[%.1f..%.1f] z[%.1f..%.1f] class=%s",
                              i, obstacle, vt, name ? name : "(null)",
                              pos.x, pos.y, pos.z,
                              hz.from_part ? "hitbump" : "gob",
                              wx0, wx1, wy0, wy1, wz0, pos.z + hz.mx.z,
                              isScenery ? "scenery" : isPost ? "post"
                              : (hz.cls == kHazardBar ? "bar" : "block"));
            } else {
                acclog::Write("SwoopRace",
                              "obstacle slot=%d ptr=%p vtable=%p name=[%s] "
                              "pos=(%.2f,%.2f,%.2f)%s",
                              i, obstacle, vt, name ? name : "(null)",
                              pos.x, pos.y, pos.z,
                              (k2 && hz.state == kHazardFailed) ? " box=FAILED" : "");
            }
        }

        if (isPost)    { ++posts_skipped;   continue; }  // wall cue covers these
        if (isScenery) { ++scenery_skipped; continue; }  // phantom boxes, never hit

        if (haveBox && hz.cls == kHazardBar) {
            // ---- Low bar: the jump-now cue; never the warn loop.
            ++bars_seen;
            SpatialAudioState::BarInfo cur;
            cur.slot = i; cur.x0 = wx0; cur.x1 = wx1; cur.y0 = wy0; cur.y1 = wy1;
            ComputeBarGap(cur, g_state.ground_z, playerRadius);
            if (newBarCount < kMgoArraySlotCount) newBars[newBarCount++] = cur;
            if (!g_state.bar_gap_diag_emitted && g_state.mine_count > 0) {
                acclog::Write("SwoopRace",
                              "bar strip: slot=%d name=[%s] y=%.0f x[%.1f..%.1f] gap=%d side=%s "
                              "target=%.1f (bikeZ=%.1f bikeR=%.2f)",
                              i, name ? name : "(null)", wy0, wx0, wx1, cur.has_gap ? 1 : 0,
                              cur.gap_side < 0 ? "left" : cur.gap_side > 0 ? "right" : "none",
                              cur.gap_x, g_state.ground_z, playerRadius);
            }
            const float gap = wy0 - (bikeOk ? bikePos.y : listener_pos.y);
            if (gap < -kObstacleForwardMargin || gap > kObstacleCueRangeM) {
                hz.jump_cued = 0;   // passed / far: re-arm
                hz.park_cued = 0;
                continue;
            }
            const float bikeX = bikeOk ? bikePos.x : listener_pos.x;
            const bool inSpan = bikeX >= wx0 - kBarLaneMarginU &&
                                bikeX <= wx1 + kBarLaneMarginU;
            if (parked) {
                hz.jump_cued = 0;   // re-arm for the relaunch
                // Parked right in front of this bar, inside its span: classify
                // the escape (see ParkedBar in the header) for the stuck hint,
                // and — off the wall, no strip — play the centred beep once per
                // park: "jump first, then W". At the wall the beep stays
                // silent (a jump there hits the pillar; rounds 15/16 lost 7 and
                // 11 relaunches that way) and the wall loop is the cue.
                if (inSpan && gap > -kBarLaneMarginU && gap <= kParkedBarAheadU &&
                    g_state.parked_bar == ParkedBar::None) {
                    if (bikeX >= kParkedWallU)       g_state.parked_bar = ParkedBar::WallRight;
                    else if (bikeX <= -kParkedWallU) g_state.parked_bar = ParkedBar::WallLeft;
                    else if (cur.has_gap)            g_state.parked_bar = ParkedBar::Strip;
                    else {
                        g_state.parked_bar = ParkedBar::MustJump;
                        if (!hz.park_cued) {
                            hz.park_cued = 1;
                            acc::audio::PlayCue(kJumpNowCueResref);
                            acclog::Write("SwoopRace",
                                          "parked beep: slot=%d name=[%s] bar y=%.1f gap=%.1f bikeX=%.2f",
                                          i, name ? name : "(null)", wy0, gap, bikeX);
                        }
                    }
                }
                continue;
            }
            hz.park_cued = 0;
            if (!inSpan || bikeSpeed <= 1.0f || gap <= 0.0f) continue;
            const float vWorld = bikeSpeed * kSpeedToWorldPerSec;   // world u/s
            const float ttc    = gap / vWorld;                       // real seconds
            // Lead: the single-bar constant, or — when the next bar behind
            // this one is close enough that one jump clears both at this
            // speed with a press window of at least kPairMinWindowSec — the
            // later "pair" lead that centres the usual reaction in that
            // window (see kJumpReactionSec). The bar list is last tick's.
            float lead = kJumpCueLeadSec;
            bool  pair = false;
            {
                float nextY0 = 0.0f, nextY1 = 0.0f;  bool haveNext = false;
                for (int b = 0; b < g_state.bar_count; ++b) {
                    const SpatialAudioState::BarInfo& nb = g_state.bars[b];
                    if (nb.slot == i || nb.y0 <= wy0) continue;
                    if (!haveNext || nb.y0 < nextY0) {
                        nextY0 = nb.y0; nextY1 = nb.y1; haveNext = true;
                    }
                }
                if (haveNext) {
                    const float reach  = (nextY1 + kBarLaneMarginU - wy0) / vWorld;
                    const float window = kJumpOverBarUntilSec - reach - kJumpOverBarFromSec;
                    if (window >= kPairMinWindowSec) {
                        lead = kJumpOverBarFromSec + 0.5f * window + kJumpReactionSec;
                        pair = true;
                    }
                }
            }
            // First beep: "press now". Second beep: the bike was airborne from
            // an earlier press and has just landed with the bar still ahead
            // inside the window — press again.
            const bool first  = hz.jump_cued == 0 && ttc <= lead;
            const bool repeat = hz.jump_cued == 1 && justLanded &&
                                ttc <= lead && ttc >= kJumpCueMinLeadSec;
            if (first || repeat) {
                ++hz.jump_cued;
                // Free side = the surviving strip (posts and mines removed);
                // none = centred, "you must jump".
                const int  side = cur.has_gap ? cur.gap_side : 0;
                // Half-bar with a strip: the beep sits at the strip's REAL
                // lateral offset, placed exactly like the guide tone (world
                // x, kSwoopGuidePanForwardRefM ahead), so its angle reads as
                // distance in the vocabulary the player already uses for pad
                // lanes — a narrow angle is "four units, lean", a wide one
                // "twelve, jump unless already moving". No strip: centred,
                // "you must jump". (Was a fixed ±5 u hard pan, which said the
                // side but never how far — asked for by the player after
                // round 14.)
                if (side == 0) {
                    acc::audio::PlayCue(kJumpNowCueResref);
                } else {
                    Vector cue = listener_pos;
                    cue.x  = cur.gap_x;
                    cue.y += kSwoopGuidePanForwardRefM;
                    acc::audio::PlayCue3DRaw(kJumpNowCueResref, cue);
                }
                acclog::Write("SwoopRace",
                              "jump cue%s: slot=%d name=[%s] bar y=%.1f x[%.1f..%.1f] "
                              "gap=%.1f speed=%.1f ttc=%.2fs lead=%.2fs pair=%d bikeX=%.2f side=%s "
                              "stripX=%.1f offset=%.1f",
                              repeat ? " (again)" : "", i, name ? name : "(null)",
                              wy0, wx0, wx1, gap, bikeSpeed, ttc, lead, pair ? 1 : 0, bikeX,
                              side == 0 ? "centre" : side < 0 ? "left" : "right",
                              side == 0 ? 0.0f : cur.gap_x,
                              side == 0 ? 0.0f : cur.gap_x - bikeX);
            }
            continue;
        }

        // ---- Block (K2) / rock (K1): the panned warn loop, admitted by the
        // forward filter + range on the hazard's REAL position. Track Y is the
        // forward axis (low → high), confirmed live on K1 (listener heartbeat
        // reaching Y≈4000 at race-end while the farthest obstacle sits at
        // Y=3742) and asserted per race on K2 by the axis probe.
        admit(i, hazardPos, name, obstacles_ahead, obstacles_in_range);
    }
    for (int b = 0; b < newBarCount; ++b) g_state.bars[b] = newBars[b];
    g_state.bar_count = newBarCount;

    // ---- KOTOR 2 mines (collected by the pad sweep this tick): blocks. ----
    for (int m = 0; m < g_state.mine_count; ++m) {
        if (admit(g_state.mine_slot[m], g_state.mine_pos[m], "mine",
                  obstacles_ahead, obstacles_in_range)) {
            ++mines_in_range;
        }
    }

    // Nearest-first selection. A straight partial selection sort: the budget is
    // single digits on KOTOR 2 and the whole array on KOTOR 1 (where the loop
    // exits immediately because there is nothing to reorder beyond the count).
    const int budget = ObstacleLoopBudget();
    const int sounding = (candidate_count < budget) ? candidate_count : budget;
    for (int a = 0; a < sounding; ++a) {
        int best = a;
        for (int b = a + 1; b < candidate_count; ++b) {
            if (candidates[b].dist < candidates[best].dist) best = b;
        }
        if (best != a) {
            ObstacleCandidate tmp = candidates[a];
            candidates[a] = candidates[best];
            candidates[best] = tmp;
        }
    }
    obstacles_dropped = candidate_count - sounding;

    for (int c = 0; c < sounding; ++c) {
        const int i = candidates[c].slot;
        // Single cue sample for all obstacles. Accelpads are no longer
        // routed through this sweep (they live in the enemies pool —
        // see TickAccelpadCues).
        const char* resref = kObstacleWarnLoopResref;

        // Decoupled cue: lateral (X) + vertical (Z) true, forward (Y)
        // pinned to a FIXED reference depth → pan encodes lane error only,
        // stable. Loudness comes from our manual curve on the REAL 3D
        // distance. See the design comment above.
        const Vector pos = candidates[c].pos;
        const float dist = candidates[c].dist;
        Vector cue_pos = pos;
        cue_pos.y = listener_pos.y + kSwoopPanForwardRefM;
        const int vol_byte = SwoopVolumeByte(dist, kObstacleCueRangeM);

        active_this_tick[i] = true;
        if (g_state.obstacle_loops[i].IsActive()) {
            // Existing loop — move it and re-assert the distance volume.
            g_state.obstacle_loops[i].UpdatePosition(cue_pos);
            g_state.obstacle_loops[i].UpdateVolume(vol_byte);
        } else {
            // First in-range tick for this obstacle — Start the loop with
            // the flat band so only our manual volume modulates loudness.
            if (g_state.obstacle_loops[i].Start(
                    resref, cue_pos, /*looping=*/true, /*spatial=*/true,
                    /*priorityGroup=*/-1, /*volumeByte=*/vol_byte,
                    /*maxVolDist=*/kSwoopFlatBandMaxM,
                    /*minVolDist=*/kSwoopFlatBandMinM)) {
                ++loops_started_this_tick;
                acclog::Trace("SwoopRace",
                              "loop start slot=%d name=[%s] "
                              "obstaclePos=(%.1f,%.1f,%.1f) "
                              "cuePos=(%.1f,%.1f,%.1f) dist=%.1f res=%s",
                              i, candidates[c].name ? candidates[c].name : "(null)",
                              pos.x, pos.y, pos.z,
                              cue_pos.x, cue_pos.y, cue_pos.z,
                              dist, resref);
            }
        }
    }

    // Stop any loops whose slot wasn't in-range this tick. Covers two
    // cases: obstacle passed > kObstacleForwardMargin behind us; or
    // (rarer) obstacle obj/aur/pos read failed this tick after having
    // succeeded last tick. Either way, Stop is idempotent so it's
    // safe to call.
    for (int i = 0; i < kMgoArraySlotCount; ++i) {
        if (!active_this_tick[i] && g_state.obstacle_loops[i].IsActive()) {
            g_state.obstacle_loops[i].Stop();
            ++loops_stopped_this_tick;
            acclog::Trace("SwoopRace",
                          "loop stop slot=%d (out of range / unresolved)", i);
        }
    }

    // Mark diagnostic done AFTER the full sweep, so we get all 22
    // obstacles' names in the log on the first iteration (not just
    // the first one).
    if (!g_state.obstacle_diag_emitted && obstacles_found > 0) {
        g_state.obstacle_diag_emitted = true;
    }
    if (!g_state.bar_gap_diag_emitted && g_state.mine_count > 0 && newBarCount > 0) {
        g_state.bar_gap_diag_emitted = true;
    }

    // Single rolling-state line. inRange = obstacles that triggered
    // a loop Start or UpdatePosition this tick; started/stopped count
    // edge transitions. At steady-state in a long stretch of nearby
    // obstacles, started and stopped should both be small or zero
    // each tick — non-zero values mark transitions.
    if (k2) {
        acclog::Trace("SwoopRace",
                      "hazards: posts=%d scenery=%d bars=%d mines=%d/%d inRange",
                      posts_skipped, scenery_skipped, bars_seen, mines_in_range,
                      g_state.mine_count);
    }
    acclog::Trace("SwoopRace",
                  "scan: slots=%d obstacles=%d ahead=%d inRange=%d (%.0fm) "
                  "overBudget=%d started=%d stopped=%d listenerY=%.1f",
                  slots_seen, obstacles_found, obstacles_ahead,
                  obstacles_in_range, kObstacleCueRangeM, obstacles_dropped,
                  loops_started_this_tick, loops_stopped_this_tick,
                  listener_pos.y);
}

}  // namespace acc::swoop_race

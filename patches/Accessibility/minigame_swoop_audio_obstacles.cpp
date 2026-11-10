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

// ============================================================================
// Continuous obstacle-proximity cue parameters.
// ============================================================================
//
//   kObstacleCueRangeM     — only obstacles within this 3D distance from
//                            the listener fire cues. Sized for ~1-1.3 s
//                            of advance warning at the top observed bike
//                            speed (~190 units/s in gear 3 on Taris), so
//                            the user has time to choose a lane before
//                            the obstacle arrives (30 m gave ~150 ms: too
//                            late).
//
//   kObstacleWarnLoopResref — engine sample looped for each obstacle.
//                            cb_gr_boncehard2 — a short metallic grenade-bounce
//                            SFX with steady pitch/volume, so looping it reads as
//                            a constant metallic clicking that can't be ignored
//                            in the ~0.7s in-range window (a sustained drone's
//                            faint attack is lost in that window).
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

namespace {

// The bike, the listener and this tick's collections, shared by the steps
// of TickObstacleCues. POD apart from the initialisers: this file is
// SEH-heavy by convention (see CLAUDE.md).
struct ObstacleTick {
    bool   k2           = false;
    Vector bikePos      = {0, 0, 0};
    bool   bikeOk       = false;
    float  bikeSpeed    = 0.0f;
    float  playerRadius = 0.0f;
    bool   justLanded   = false;
    bool   parked       = false;
    Vector listener     = {0, 0, 0};

    // In-range warn-loop hazards, before the nearest-N cut.
    struct Candidate {
        int         slot;
        float       dist;
        Vector      pos;
        const char* name;
    };
    Candidate candidates[kMgoArraySlotCount];
    int candidateCount = 0;

    // This tick's bars (K2), published to g_state after the sweep so the pair
    // lookup inside it reads a consistent (last tick's) list.
    SpatialAudioState::BarInfo newBars[kMgoArraySlotCount];
    int newBarCount = 0;

    int slotsSeen = 0, obstaclesFound = 0, ahead = 0, inRange = 0;
    int postsSkipped = 0, scenerySkipped = 0, barsSeen = 0, minesInRange = 0;
};

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

// One obstacle as the steps see it. K2 boxes are unrotated on straight +Y
// tracks (the axis probe has asserted +Y), so the world box is the origin
// plus the local box.
struct ObstacleView {
    void*       obstacle  = nullptr;
    Vector      pos       = {0, 0, 0};
    const char* name      = nullptr;
    bool        haveBox   = false;
    float       x0 = 0.0f, x1 = 0.0f, y0 = 0.0f, y1 = 0.0f, z0 = 0.0f;
    bool        scenery   = false;
    bool        post      = false;
    // Where the hazard really is: box centre laterally (clamped to the
    // lane), leading edge forward, base height.
    Vector      hazardPos = {0, 0, 0};
};

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

// Parked right in front of this bar, inside its span: classify the escape
// (see ParkedBar in the header) for the stuck hint, and, off the wall with
// no strip, play the centred beep once per park: "jump first, then W". At
// the wall the beep stays silent (a jump there hits the pillar; rounds 15/16
// lost 7 and 11 relaunches that way) and the wall loop is the cue.
void HandleParkedBar(int i, const ObstacleView& v, HazardInfo& hz,
                     const SpatialAudioState::BarInfo& cur, float gap, float bikeX) {
    if (gap <= -kBarLaneMarginU || gap > kParkedBarAheadU ||
        g_state.parked_bar != ParkedBar::None) {
        return;
    }
    if (bikeX >= kParkedWallU) {
        g_state.parked_bar = ParkedBar::WallRight;
    } else if (bikeX <= -kParkedWallU) {
        g_state.parked_bar = ParkedBar::WallLeft;
    } else if (cur.has_gap) {
        g_state.parked_bar = ParkedBar::Strip;
    } else {
        g_state.parked_bar = ParkedBar::MustJump;
        if (!hz.park_cued) {
            hz.park_cued = 1;
            acc::audio::PlayCue(kJumpNowCueResref);
            acclog::Write("SwoopRace",
                          "parked beep: slot=%d name=[%s] bar y=%.1f gap=%.1f bikeX=%.2f",
                          i, v.name ? v.name : "(null)", v.y0, gap, bikeX);
        }
    }
}

// The jump lead: the single-bar constant, or, when the next bar behind this
// one is close enough that one jump clears both at this speed with a press
// window of at least kPairMinWindowSec, the later "pair" lead that centres
// the usual reaction in that window (see kJumpReactionSec). The bar list is
// last tick's.
float JumpLead(int slot, float y0, float vWorld, bool& pair) {
    pair = false;
    float nextY0 = 0.0f, nextY1 = 0.0f;
    bool haveNext = false;
    for (int b = 0; b < g_state.bar_count; ++b) {
        const SpatialAudioState::BarInfo& nb = g_state.bars[b];
        if (nb.slot == slot || nb.y0 <= y0) continue;
        if (!haveNext || nb.y0 < nextY0) {
            nextY0 = nb.y0;
            nextY1 = nb.y1;
            haveNext = true;
        }
    }
    if (!haveNext) return kJumpCueLeadSec;
    const float reach  = (nextY1 + kBarLaneMarginU - y0) / vWorld;
    const float window = kJumpOverBarUntilSec - reach - kJumpOverBarFromSec;
    if (window < kPairMinWindowSec) return kJumpCueLeadSec;
    pair = true;
    return kJumpOverBarFromSec + 0.5f * window + kJumpReactionSec;
}

// First beep: "press now". Second beep: the bike was airborne from an
// earlier press and has just landed with the bar still ahead inside the
// window: press again.
void MaybeJumpCue(int i, const ObstacleView& v, HazardInfo& hz,
                  const SpatialAudioState::BarInfo& cur, float gap, float bikeX,
                  const ObstacleTick& t) {
    const float vWorld = t.bikeSpeed * kSpeedToWorldPerSec;  // world u/s
    const float ttc    = gap / vWorld;                       // real seconds
    bool pair = false;
    const float lead = JumpLead(i, v.y0, vWorld, pair);
    const bool first  = hz.jump_cued == 0 && ttc <= lead;
    const bool repeat = hz.jump_cued == 1 && t.justLanded &&
                        ttc <= lead && ttc >= kJumpCueMinLeadSec;
    if (!first && !repeat) return;
    ++hz.jump_cued;
    // Free side = the surviving strip (posts and mines removed). With a strip
    // the beep sits at the strip's real lateral offset, placed like the guide
    // tone (world x, kSwoopGuidePanForwardRefM ahead), so its angle reads as
    // distance in the vocabulary of the pad lanes: narrow is "four units,
    // lean", wide "twelve, jump unless already moving". No strip: centred,
    // "you must jump".
    const int side = cur.has_gap ? cur.gap_side : 0;
    if (side == 0) {
        acc::audio::PlayCue(kJumpNowCueResref);
    } else {
        Vector cue = t.listener;
        cue.x  = cur.gap_x;
        cue.y += kSwoopGuidePanForwardRefM;
        acc::audio::PlayCue3DRaw(kJumpNowCueResref, cue);
    }
    acclog::Write("SwoopRace",
                  "jump cue%s: slot=%d name=[%s] bar y=%.1f x[%.1f..%.1f] "
                  "gap=%.1f speed=%.1f ttc=%.2fs lead=%.2fs pair=%d bikeX=%.2f side=%s "
                  "stripX=%.1f offset=%.1f",
                  repeat ? " (again)" : "", i, v.name ? v.name : "(null)",
                  v.y0, v.x0, v.x1, gap, t.bikeSpeed, ttc, lead, pair ? 1 : 0, bikeX,
                  side == 0 ? "centre" : side < 0 ? "left" : "right",
                  side == 0 ? 0.0f : cur.gap_x,
                  side == 0 ? 0.0f : cur.gap_x - bikeX);
}

// A low bar (K2): the jump-now cue, never the warn loop.
void HandleBar(int i, const ObstacleView& v, HazardInfo& hz, ObstacleTick& t) {
    ++t.barsSeen;
    SpatialAudioState::BarInfo cur;
    cur.slot = i; cur.x0 = v.x0; cur.x1 = v.x1; cur.y0 = v.y0; cur.y1 = v.y1;
    ComputeBarGap(cur, g_state.ground_z, t.playerRadius);
    if (t.newBarCount < kMgoArraySlotCount) t.newBars[t.newBarCount++] = cur;
    if (!g_state.bar_gap_diag_emitted && g_state.mine_count > 0) {
        acclog::Write("SwoopRace",
                      "bar strip: slot=%d name=[%s] y=%.0f x[%.1f..%.1f] gap=%d side=%s "
                      "target=%.1f (bikeZ=%.1f bikeR=%.2f)",
                      i, v.name ? v.name : "(null)", v.y0, v.x0, v.x1, cur.has_gap ? 1 : 0,
                      cur.gap_side < 0 ? "left" : cur.gap_side > 0 ? "right" : "none",
                      cur.gap_x, g_state.ground_z, t.playerRadius);
    }
    const float gap = v.y0 - (t.bikeOk ? t.bikePos.y : t.listener.y);
    if (gap < -kObstacleForwardMargin || gap > kObstacleCueRangeM) {
        hz.jump_cued = 0;   // passed or far: re-arm
        hz.park_cued = 0;
        return;
    }
    const float bikeX = t.bikeOk ? t.bikePos.x : t.listener.x;
    const bool inSpan = bikeX >= v.x0 - kBarLaneMarginU && bikeX <= v.x1 + kBarLaneMarginU;
    if (t.parked) {
        hz.jump_cued = 0;   // re-arm for the relaunch
        if (inSpan) HandleParkedBar(i, v, hz, cur, gap, bikeX);
        return;
    }
    hz.park_cued = 0;
    if (!inSpan || t.bikeSpeed <= 1.0f || gap <= 0.0f) return;
    MaybeJumpCue(i, v, hz, cur, gap, bikeX, t);
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

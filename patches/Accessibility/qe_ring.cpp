// Q/E all around — see qe_ring.h.
#include "qe_ring.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "engine_area.h"
#include "engine_compass.h"
#include "engine_panels.h"
#include "engine_player.h"
#include "engine_reads.h"
#include "examine_view.h"         // IsHostileCreature
#include "filter_objects.h"
#include "log.h"
#include "menus_modsettings.h"
#include "minigame_swoop_race.h"
#include "minigame_turret.h"
#include "narrated_target.h"
#include "passive_narrate.h"
#include "speech.h"
#include "spatial_change_detector.h"  // GetCachedWalls
#include "strings.h"

namespace acc::qe_ring {

namespace {

using acc::filter::CycleCategory;

// The engine's Q/E candidate range (SelectNearestObject's 30 units).
constexpr float kRangeM = 30.0f;
// The line-of-sight test stops this short of the object, so a container or
// panel standing against a wall is not hidden by that wall.
constexpr float kLosShortenM = 0.6f;
// A placeable set into the wall itself (an escape pod, a wall panel) has its
// position past the walkmesh edge. When the line first meets a wall this
// close to such an object, the wall is the one it is built into, not one in
// between. 2026-11-08, Endar Spire: the Escape Pod 5 m away never came up on
// Q/E, so the player had no way to pick it and was stuck.
constexpr float kWallSetPlaceableM = 2.0f;

struct Candidate {
    void*    obj;
    uint32_t handle;
    float    angle;  // clockwise from straight ahead, 0..360
    float    dist;
};

CycleCategory Classify(void* obj) {
    const CycleCategory order[] = {CycleCategory::Npc, CycleCategory::Door,
                                   CycleCategory::Container, CycleCategory::Item,
                                   CycleCategory::Transition};
    for (CycleCategory c : order) {
        if (acc::filter::ObjectMatches(obj, c)) return c;
    }
    return CycleCategory::Count_;
}

bool IsParty(uint32_t handle, const uint32_t* party, int count) {
    for (int i = 0; i < count; ++i) {
        if (party[i] == handle) return true;
    }
    return false;
}

}  // namespace

bool Enabled() {
    return acc::menus::modsettings::GetToggle(
        acc::menus::modsettings::Option::QEAllAround);
}

bool HandlePress(int direction) {
    if (!Enabled()) return false;
    Vector player;
    if (!acc::engine::GetPlayerPosition(player)) return false;
    if (acc::engine::IsForegroundUiBlocking() ||
        acc::engine::HasActiveDialogPanel() ||
        acc::turret_game::IsActive() || acc::swoop_race::IsActive()) {
        return false;
    }
    void* area = acc::engine::GetCurrentArea();
    if (!area) return false;

    float yaw = 0.0f;
    const bool haveYaw = acc::engine::GetPlayerYawDegrees(yaw);
    const float facing = haveYaw ? acc::engine::EngineYawToCompass(yaw) : 0.0f;
    const bool combat = acc::engine::IsAnyPartyMemberInCombat();

    uint32_t party[12] = {0};
    const int partyCount = acc::engine::GetPartyMembers(party, 12);
    void* pc = acc::engine::GetPlayerServerCreature();

    const acc::engine::WallEdge* walls = nullptr;
    int wallCount = 0;
    const bool haveWalls =
        acc::spatial::change_detector::GetCachedWalls(walls, wallCount) &&
        walls && wallCount > 0;

    std::vector<Candidate> cands;
    acc::engine::AreaObjectIterator it(area);
    while (void* obj = it.Next()) {
        if (obj == pc) continue;
        const CycleCategory cat = Classify(obj);
        if (cat == CycleCategory::Count_) continue;
        if (combat && cat != CycleCategory::Npc) continue;
        Vector p;
        if (!acc::engine::GetObjectPosition(obj, p)) continue;
        const float dx = p.x - player.x;
        const float dy = p.y - player.y;
        const float dist = std::sqrt(dx * dx + dy * dy);
        if (dist > kRangeM || dist < 0.05f) continue;
        const uint32_t handle = acc::engine::GetObjectHandle(obj);
        if (cat == CycleCategory::Npc) {
            if (IsParty(handle, party, partyCount)) continue;
            const int hp = acc::engine::ReadCreatureCurrentHitPoints(obj);
            if (hp == 0 || hp < -1) continue;  // dead
            if (acc::examine_view::IsCreatureDead(obj)) continue;
            if (combat && !acc::examine_view::IsHostileCreature(obj)) continue;
        }
        if (cat == CycleCategory::Door && acc::engine::IsDoorCosmetic(obj)) continue;
        if (haveWalls && dist > kLosShortenM) {
            const float k = (dist - kLosShortenM) / dist;
            Vector end{player.x + dx * k, player.y + dy * k, p.z};
            Vector hit{};
            if (acc::engine::SegmentCrossesWalkmesh(walls, wallCount, player, end,
                                                    hit)) {
                const float hx = p.x - hit.x;
                const float hy = p.y - hit.y;
                const bool setInWall =
                    cat == CycleCategory::Container &&
                    hx * hx + hy * hy <= kWallSetPlaceableM * kWallSetPlaceableM;
                if (!setInWall) continue;  // behind a wall
            }
        }
        const float bearing = acc::engine::EngineYawToCompass(
            std::atan2(dy, dx) * 57.29577951308232f);
        float rel = std::fmod(bearing - facing + 720.0f, 360.0f);
        if (rel >= 359.5f) rel = 0.0f;
        cands.push_back(Candidate{obj, handle, rel, dist});
    }

    if (cands.empty()) {
        acc::narrated_target::Clear();
        acc::speech::Speak(acc::strings::Get(acc::strings::Id::CycleNoTarget),
                     /*interrupt=*/true);
        acclog::Write("QERing", "dir=%d: no candidates (combat=%d)", direction,
                      combat ? 1 : 0);
        return true;
    }

    std::sort(cands.begin(), cands.end(),
              [](const Candidate& a, const Candidate& b) {
                  if (a.angle != b.angle) return a.angle < b.angle;
                  if (a.dist != b.dist) return a.dist < b.dist;
                  return a.handle < b.handle;
              });

    int cur = -1;
    acc::narrated_target::Slot slot;
    if (acc::narrated_target::TryGet(slot) && !slot.isMapPin) {
        for (size_t i = 0; i < cands.size(); ++i) {
            if (cands[i].handle == slot.handle) {
                cur = static_cast<int>(i);
                break;
            }
        }
    }
    const int n = static_cast<int>(cands.size());
    int next;
    if (cur < 0) {
        next = direction > 0 ? 0 : n - 1;  // nearest to straight ahead
    } else {
        next = ((cur + (direction > 0 ? 1 : -1)) % n + n) % n;
    }

    const Candidate& c = cands[next];
    acc::passive_narrate::NarrateExplicit(c.handle);
    acclog::Write("QERing", "dir=%d: %d of %d, handle 0x%08x at %.0f deg, %.1f m",
                  direction, next + 1, n, c.handle, c.angle, c.dist);
    return true;
}

}  // namespace acc::qe_ring

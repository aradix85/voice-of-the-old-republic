// Deadly trigger areas as standing zones (deadly_triggers.h).

#include "deadly_triggers.h"

#include <cmath>
#include <cstring>
#include <iterator>

#include "engine_area.h"
#include "turnback_zones.h"

namespace acc::guidance::deadly {

namespace {

// Tags are matched without regard to case; the deadly ones by their start,
// since such triggers come numbered.
struct Kind {
    const char* tagStart;  // the triggers that kill
    const char* gateTag;   // the trigger that opens the way through them
};

constexpr Kind kKinds[] = {
    // unk_m41ab: blowtrig1..5 and blowtrigd1..6 (scripts blow1.., blowd1..:
    // EffectDamage 998 and EffectDeath on whoever enters). talktrig
    // (cp_unk41ab_hotlk) starts the hologram and destroys blowtrig1..5.
    {"blowtrig", "talktrig"},
};

// A zone reaches this far beyond its trigger: the engine's walk is not exact
// to the decimetre, and the companions walk beside the leader. Small enough
// to leave the opening between the pylons (5 m) open.
constexpr float kMarginM = 1.0f;

bool   g_haveGate = false;
Vector g_gate{};

// Moves every corner kMarginM away from the middle of the polygon.
void Widen(Vector* poly, int count) {
    if (count < 3) return;
    float cx = 0.0f, cy = 0.0f;
    for (int i = 0; i < count; ++i) {
        cx += poly[i].x;
        cy += poly[i].y;
    }
    cx /= static_cast<float>(count);
    cy /= static_cast<float>(count);
    for (int i = 0; i < count; ++i) {
        const float dx = poly[i].x - cx;
        const float dy = poly[i].y - cy;
        const float d  = std::sqrt(dx * dx + dy * dy);
        if (d < 1e-3f) continue;
        poly[i].x += dx / d * kMarginM;
        poly[i].y += dy / d * kMarginM;
    }
}

}  // namespace

void Refresh(void* area) {
    turnback::ForArea(area);
    turnback::ClearStanding();
    g_haveGate = false;
    if (!area) return;

    bool   found[std::size(kKinds)]    = {};
    bool   haveGate[std::size(kKinds)] = {};
    Vector gate[std::size(kKinds)]     = {};

    acc::engine::AreaObjectIterator it(area);
    while (void* obj = it.Next()) {
        if (acc::engine::GetObjectKind(obj) !=
            static_cast<int>(acc::engine::GameObjectKind::Trigger)) {
            continue;
        }
        char tag[64] = "";
        if (!acc::engine::GetObjectTag(obj, tag, sizeof(tag))) continue;
        for (size_t k = 0; k < std::size(kKinds); ++k) {
            if (_strnicmp(tag, kKinds[k].tagStart, std::strlen(kKinds[k].tagStart)) == 0) {
                Vector poly[turnback::kMaxVerts];
                const int n = acc::engine::GetTriggerGeometry(obj, poly, turnback::kMaxVerts);
                Widen(poly, n);
                if (turnback::AddStanding(poly, n)) found[k] = true;
            } else if (_stricmp(tag, kKinds[k].gateTag) == 0) {
                haveGate[k] = acc::engine::GetTriggerCentroid(obj, gate[k]) > 0;
            }
        }
    }
    for (size_t k = 0; k < std::size(kKinds); ++k) {
        if (found[k] && haveGate[k]) {
            g_haveGate = true;
            g_gate     = gate[k];
            return;
        }
    }
}

bool Gate(Vector& out) {
    if (!g_haveGate) return false;
    out = g_gate;
    return true;
}

}  // namespace acc::guidance::deadly

// Door lookups near a point for the exit-memory modules: locked doors, the
// closed or any door at a spot, and the nearest door into another area.

#include "exit_memory_internal.h"

namespace acc::exit_memory {

std::vector<Vector> LockedDoorPositions(void* area) {
    std::vector<Vector> out;
    acc::engine::AreaObjectIterator it(area);
    while (void* obj = it.Next()) {
        if (acc::engine::GetObjectKind(obj) !=
            static_cast<int>(acc::engine::GameObjectKind::Door)) {
            continue;
        }
        if (!acc::engine::IsDoorLocked(obj) || acc::engine::IsDoorOpen(obj)) {
            continue;
        }
        Vector p;
        if (acc::engine::GetObjectPosition(obj, p)) out.push_back(p);
    }
    return out;
}

// A closed door standing within kDoorMatchM of `p`, or null.
void* ClosedDoorNear(void* area, const Vector& p) {
    acc::engine::AreaObjectIterator it(area);
    while (void* obj = it.Next()) {
        if (acc::engine::GetObjectKind(obj) !=
            static_cast<int>(acc::engine::GameObjectKind::Door)) {
            continue;
        }
        if (acc::engine::IsDoorOpen(obj)) continue;
        Vector q;
        if (!acc::engine::GetObjectPosition(obj, q)) continue;
        const float dx = q.x - p.x;
        const float dy = q.y - p.y;
        if (dx * dx + dy * dy <= kDoorMatchM * kDoorMatchM) return obj;
    }
    return nullptr;
}

// Any door (open or closed) standing within kDoorMatchM of `p`, or null.
void* DoorNear(void* area, const Vector& p) {
    acc::engine::AreaObjectIterator it(area);
    while (void* obj = it.Next()) {
        if (acc::engine::GetObjectKind(obj) !=
            static_cast<int>(acc::engine::GameObjectKind::Door)) {
            continue;
        }
        Vector q;
        if (!acc::engine::GetObjectPosition(obj, q)) continue;
        const float dx = q.x - p.x;
        const float dy = q.y - p.y;
        if (dx * dx + dy * dy <= kDoorMatchM * kDoorMatchM) return obj;
    }
    return nullptr;
}

// The nearest door into another area within `maxM` of `p`, or null. With a
// non-empty `destName` only a door whose destination reads exactly that.
void* TransitionDoorNear(void* area, const Vector& p, float maxM,
                         const std::string& destName, Vector* outPos) {
    void* best = nullptr;
    float bestSq = maxM * maxM;
    acc::engine::AreaObjectIterator it(area);
    while (void* obj = it.Next()) {
        if (acc::engine::GetObjectKind(obj) !=
            static_cast<int>(acc::engine::GameObjectKind::Door)) {
            continue;
        }
        if (!acc::engine::DoorHasTransitionDest(obj)) continue;
        Vector q;
        if (!acc::engine::GetObjectPosition(obj, q)) continue;
        const float dx = q.x - p.x;
        const float dy = q.y - p.y;
        const float d2 = dx * dx + dy * dy;
        if (d2 > bestSq) continue;
        if (!destName.empty()) {
            char dest[128] = {0};
            acc::engine::ExtractTextOrStrRef(
                obj, ::kDoorTransitionDestOffset,
                ::kDoorTransitionDestOffset + 4, dest, sizeof(dest));
            if (destName != dest) continue;
        }
        best = obj;
        bestSq = d2;
        if (outPos) *outPos = q;
    }
    return best;
}

}  // namespace acc::exit_memory

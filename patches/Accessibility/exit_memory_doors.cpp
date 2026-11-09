// Door lookups near a point for the exit-memory modules: locked doors, the
// closed or any door at a spot, and the nearest door into another area.

#include "exit_memory_internal.h"

#include "engine_area.h"
#include "engine_reads.h"      // ReadCreatureCurrentHitPoints

namespace acc::exit_memory {

std::vector<Vector> LockedDoorPositions(void* area) {
    std::vector<Vector> out;
    acc::engine::ForEachDoor(area, [&](void* door, const Vector& pos) {
        if (acc::engine::IsDoorLocked(door) && !acc::engine::IsDoorOpen(door)) {
            out.push_back(pos);
        }
    });
    return out;
}

// The closed door within kDoorMatchM of `p`, or null.
void* ClosedDoorNear(void* area, const Vector& p) {
    return acc::engine::NearestDoor(area, p, kDoorMatchM, [](void* door) {
        return !acc::engine::IsDoorOpen(door);
    });
}

// The door (open or closed) within kDoorMatchM of `p`, or null.
void* DoorNear(void* area, const Vector& p) {
    return acc::engine::NearestDoor(area, p, kDoorMatchM, [](void*) { return true; });
}

// The nearest door into another area within `maxM` of `p`, or null. With a
// non-empty `destName` only a door whose destination reads exactly that.
void* TransitionDoorNear(void* area, const Vector& p, float maxM,
                         const std::string& destName, Vector* outPos) {
    return acc::engine::NearestDoor(area, p, maxM, [&](void* door) {
        if (!acc::engine::DoorHasTransitionDest(door)) return false;
        if (destName.empty()) return true;
        char dest[128] = {0};
        acc::engine::ExtractTextOrStrRef(door, ::kDoorTransitionDestOffset,
                                         ::kDoorTransitionDestOffset + 4,
                                         dest, sizeof(dest));
        return destName == dest;
    }, outPos);
}

}  // namespace acc::exit_memory

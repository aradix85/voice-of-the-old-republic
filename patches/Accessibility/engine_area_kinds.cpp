// What kind of object this is: a usable placeable, an empty container and its item count, a landmark waypoint, a transition trigger and its geometry, a detected trap, and an object's local booleans.

#include "engine_area_internal.h"

namespace acc::engine {

bool IsUsablePlaceable(void* placeable) {
    if (!placeable) return false;
    __try {
        auto* base = reinterpret_cast<unsigned char*>(placeable);
        uint8_t usable       = *(base + kPlaceableUsableOffset);
        uint8_t hasInventory = *(base + kPlaceableHasInventoryOffset);
        return usable != 0 || hasInventory != 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool IsEmptyContainer(void* gameObject) {
    if (!gameObject) return false;
    // Gate on kind FIRST: has_inventory / item_repository live at
    // placeable offsets that map to unrelated fields on other object
    // kinds, so reading them on a creature/door could fake an empty
    // container.
    int kind = GetObjectKind(gameObject);
    if (kind != static_cast<int>(GameObjectKind::Placeable)) {
        return false;
    }
    __try {
        auto* base = reinterpret_cast<unsigned char*>(gameObject);
        // Only true loot containers qualify; switches / computer panels and
        // other usable-but-not-lootable placeables carry HasInventory == 0
        // and a null repository, and must never get an "empty" tag.
        if (*reinterpret_cast<int*>(base + kPlaceableHasInventoryOffset) == 0) {
            return false;
        }
        void* repo = *reinterpret_cast<void**>(
            base + kPlaceableItemRepositoryOffset);
        if (!repo) return false;
        int count = *reinterpret_cast<int*>(
            reinterpret_cast<unsigned char*>(repo) +
            kItemRepositoryItemCountOffset);
        return count <= 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool IsLandmarkWaypoint(void* waypoint) {
    if (!waypoint) return false;
    __try {
        return *(reinterpret_cast<unsigned char*>(waypoint) +
                 kWaypointHasMapNoteOffset) != 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool IsTransitionTrigger(void* trigger) {
    if (!trigger) return false;
    __try {
        // CSWSTrigger.transition_destination: inline text pointer at +0
        // with length-or-strref at +4 (same dual-slot convention
        // ExtractTextOrStrRef consumes for doors). A trigger with no
        // destination has a null text pointer and the GFF "no strref"
        // sentinel 0xFFFFFFFF in the +4 slot.
        //
        // The old probe read the first 12 bytes as a Vector and tested
        // != 0.0f — but the 0xFFFFFFFF sentinel decodes as NaN, and
        // NaN != 0.0f is true, so EVERY destination-less trigger (traps,
        // banter/dialogue triggers, shield triggers) classified as a
        // transition: they polluted the transition cycling category and
        // fired the Transition proximity cue on approach (2026-07-16
        // Southern Strand session). Mirror ExtractTextOrStrRef's
        // resolution order instead, with LookupTlk's strref validity
        // bounds, minus the actual TLK call (this predicate runs per
        // object per tick in the change detector).
        unsigned char* p = reinterpret_cast<unsigned char*>(trigger) +
                           kTriggerTransitionDestOffset;
        const char* text = *reinterpret_cast<const char**>(p);
        uint32_t    aux  = *reinterpret_cast<uint32_t*>(p + 4);
        // Inline text: aux is its length. 0xFFFFFFFF is never a length: it
        // is the "no strref" sentinel next to a non-null (empty) text slot,
        // which is what plain script triggers carry (Endar Spire's
        // "nulltrigger", "Trask on Bridge" and the explosion triggers all
        // have TransitionDestination strref -1 and no LinkedTo, yet were
        // cycled and Q/E-listed as exits, 2026-11-08).
        if (text) return aux > 0 && aux <= 0x10000u;
        // No inline text: aux is the TLK strref (0 / -1 / out-of-range
        // all mean "no destination", matching LookupTlk).
        return aux != 0 && aux <= 0x100000u;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

int GetTriggerGeometry(void* trigger, Vector* out, int maxVerts) {
    if (!trigger || !out || maxVerts <= 0) return 0;
    __try {
        unsigned char* p = reinterpret_cast<unsigned char*>(trigger);
        int count = *reinterpret_cast<int*>(p + kTriggerGeometryCountOffset);
        Vector* verts =
            *reinterpret_cast<Vector**>(p + kTriggerGeometryOffset);
        // Authored polygons are 3..8 vertices; anything outside a sane
        // range is a mis-read (wrong object kind / stale pointer), not a
        // real shape.
        if (!verts || count < 3 || count > 32) return 0;
        if (count > maxVerts) count = maxVerts;
        for (int i = 0; i < count; ++i) out[i] = verts[i];
        return count;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

bool GetObjectLocalBoolean(void* gameObject, int index) {
    if (!gameObject || index < 0 || index >= 96) return false;
    __try {
        uint32_t* words = reinterpret_cast<uint32_t*>(
            reinterpret_cast<unsigned char*>(gameObject) +
            kObjectVarTableOffset);
        return (words[index >> 5] & (1u << (index & 31))) != 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool TrapDetectedByAnyOf(void* gameObject,
                         const uint32_t* ids, int idCount) {
    if (!gameObject || !ids || idCount <= 0) return false;
    size_t listOff = 0;
    int kind = GetObjectKind(gameObject);
    if (kind == static_cast<int>(GameObjectKind::Trigger)) {
        listOff = kTriggerTrapDetectedListOffset;
    } else if (kind == static_cast<int>(GameObjectKind::Door)) {
        listOff = kDoorTrapDetectedListOffset;
    } else if (kind == static_cast<int>(GameObjectKind::Placeable)) {
        listOff = kPlaceableTrapDetectedListOffset;
    } else {
        return false;
    }
    __try {
        unsigned char* base = reinterpret_cast<unsigned char*>(gameObject);
        uint32_t* data  = *reinterpret_cast<uint32_t**>(base + listOff);
        int       count = *reinterpret_cast<int*>(base + listOff + 4);
        if (!data || count <= 0) return false;
        if (count > 64) count = 64;  // sanity cap on a corrupt read
        for (int i = 0; i < count; ++i) {
            for (int j = 0; j < idCount; ++j) {
                if (data[i] == ids[j]) return true;
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    return false;
}

int GetContainerItemCount(void* gameObject) {
    if (!gameObject ||
        GetObjectKind(gameObject) != static_cast<int>(GameObjectKind::Placeable)) {
        return -1;
    }
    __try {
        auto* base = reinterpret_cast<unsigned char*>(gameObject);
        if (*reinterpret_cast<int*>(base + kPlaceableHasInventoryOffset) == 0) {
            return -1;
        }
        void* repo = *reinterpret_cast<void**>(
            base + kPlaceableItemRepositoryOffset);
        if (!repo) return -1;
        return *reinterpret_cast<int*>(
            reinterpret_cast<unsigned char*>(repo) +
            kItemRepositoryItemCountOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

}  // namespace acc::engine

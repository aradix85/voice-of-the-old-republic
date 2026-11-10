// Door state: open, locked, static, cosmetic, a transition destination, the doors that were locked when the area loaded, and the door part of a spoken name.

#include "engine_area_internal.h"

#include "log.h"            // seam-filter telemetry
#include "strings.h"        // door state suffix lookup (DoorOpen/DoorLocked)
#include "engine_game.h"    // IsKotor2 — K2 flags real locked doors Static

namespace acc::engine {

namespace {

// Door lock snapshot — which doors were locked when the area came up.
//
// Narration numbers doors within a class, and the class has to be frozen:
// derive it from the live lock flag and every unlock renumbers the area.
// Only the locked handles are stored (the interesting minority); anything
// absent from the table counts as "was not locked".
constexpr int kMaxSnapshotDoors = 256;
uint32_t s_lockedAtLoad[kMaxSnapshotDoors];
int      s_lockedAtLoadCount = 0;
void*    s_lockSnapshotArea  = nullptr;  // also the "snapshot exists" flag

// One SEH-guarded read of the two live door state fields. Both callers
// (the suffix builder and the lock snapshot) want them under one guard.
// Leaves the outputs untouched and returns false on fault.
bool ReadDoorFlags(void* serverDoor, uint32_t& locked, uint8_t& openState) {
    if (!serverDoor) return false;
    __try {
        auto* base = reinterpret_cast<unsigned char*>(serverDoor);
        locked    = *reinterpret_cast<uint32_t*>(base + kDoorLockedOffset);
        openState = *reinterpret_cast<uint8_t*> (base + kDoorOpenStateOffset);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void BuildDoorLockSnapshot(void* area) {
    s_lockedAtLoadCount = 0;
    s_lockSnapshotArea  = area;
    if (!area) return;

    int doors = 0;
    AreaObjectIterator it(area);
    while (void* obj = it.Next()) {
        if (GetObjectKind(obj) !=
            static_cast<int>(GameObjectKind::Door)) {
            continue;
        }
        ++doors;
        uint32_t locked = 0;
        uint8_t  open   = 0;
        if (!ReadDoorFlags(obj, locked, open) || locked == 0) continue;
        if (s_lockedAtLoadCount >= kMaxSnapshotDoors) {
            acclog::Write("DoorLockSnapshot",
                "area=%p exceeded %d locked doors — remainder counts as "
                "unlocked for numbering", area, kMaxSnapshotDoors);
            break;
        }
        s_lockedAtLoad[s_lockedAtLoadCount++] = GetObjectHandle(obj);
    }
    acclog::Write("DoorLockSnapshot",
        "area=%p doors=%d lockedAtLoad=%d", area, doors, s_lockedAtLoadCount);
}

}  // namespace

// Build the comma-prefixed suffix for a CSWSDoor — state + transition
// destination + description. Empty when the door is in the boring default
// state (closed, and never locked here) AND has no transition target /
// description.
//
// Order is deliberate: state first ("Door, locked") because that's the
// most actionable bit for the player; destination second ("Door, open,
// Bridge") so the user hears it before any long-form description; then
// description last.
void BuildDoorSuffix(void* serverDoor, char* outBuf, size_t bufSize) {
    if (!outBuf || bufSize == 0) return;
    outBuf[0] = '\0';
    if (!serverDoor) return;

    // Static doors are non-interactive set dressing — the engine never lets
    // anyone open them (it offers no actions at all). They're frequently also
    // flagged locked in the blueprint, but "locked" misleads the player
    // into hunting for a key/slice that doesn't exist. Label them "cosmetic"
    // instead and skip the rest of the suffix (state/transition/description are
    // all meaningless on a door that can't be used). IsDoorCosmetic carries
    // the KOTOR 2 exception: Obsidian flags real, lock-bearing doors Static
    // too (Harbinger HammerHeadDoor2: Static=1 + Locked=1 + OpenLockDC in the
    // template, and the engine still ran its lock — a mine opened it in the
    // 2026-08-11 session), so lock evidence beats the Static flag there.
    if (IsDoorCosmetic(serverDoor)) {
        AppendCommaSeparated(outBuf, bufSize,
            acc::strings::Get(acc::strings::Id::DoorCosmetic));
        return;
    }

    uint32_t locked    = 0;
    uint8_t  openState = 0;
    ReadDoorFlags(serverDoor, locked, openState);  // defaults on fault

    // Open wins over locked: a story script can open a door without
    // clearing its lock flag (the Taris Upper City Cantina entrance stood
    // open while still Locked=1, and was read "locked" as the player walked
    // through it).
    if (openState != 0) {
        // Treat any non-zero open_state as "open enough to mention".
        // Stock KOTOR uses 0=closed, ≥1=opening/open in our observations
        // (CSWCDoor.state=2 was the post-open value seen in the May 5
        // logs); the server-side byte mirrors that. If the value space
        // turns out wider, the worst case is we read "open" for an
        // animating door — better signal than silent.
        AppendCommaSeparated(outBuf, bufSize,
            acc::strings::Get(acc::strings::Id::DoorOpen));
    } else if (locked != 0) {
        AppendCommaSeparated(outBuf, bufSize,
            acc::strings::Get(acc::strings::Id::DoorLocked));
        AppendLockNeed(serverDoor, outBuf, bufSize);
    } else if (WasDoorLockedAtAreaLoad(serverDoor)) {
        // Closed again / still closed, but this door WAS locked when we got
        // here — the player has picked or keyed it since. Worth saying: it
        // confirms the unlock landed, and it explains why the door still
        // counts in the locked-door numbering sequence. Short-lived in
        // practice (the player normally opens it a beat later, and "open"
        // then wins above).
        AppendCommaSeparated(outBuf, bufSize,
            acc::strings::Get(acc::strings::Id::DoorUnlocked));
    }

    char buf[128];
    if (TryReadLocString(serverDoor, kDoorTransitionDestOffset,
                         buf, sizeof(buf)) && buf[0]) {
        AppendCommaSeparated(outBuf, bufSize, buf);
    }
    if (TryReadLocString(serverDoor, kDoorDescriptionOffset,
                         buf, sizeof(buf)) && buf[0]) {
        AppendCommaSeparated(outBuf, bufSize, buf);
    }
}

bool WasDoorLockedAtAreaLoad(void* serverDoor) {
    if (!serverDoor) return false;
    if (GetObjectKind(serverDoor) !=
        static_cast<int>(GameObjectKind::Door)) {
        return false;
    }

    void* area = GetCurrentArea();
    // Rebuild on the first query of a new area. When the area is momentarily
    // unresolvable (mid-load) keep answering from the snapshot we have rather
    // than throwing it away — a door queried during the hand-off would
    // otherwise flip numbering class for one frame.
    if (area && area != s_lockSnapshotArea) {
        BuildDoorLockSnapshot(area);
    }
    if (!s_lockSnapshotArea) return false;

    uint32_t handle = GetObjectHandle(serverDoor);
    if (handle == 0u) return false;
    for (int i = 0; i < s_lockedAtLoadCount; ++i) {
        if (s_lockedAtLoad[i] == handle) return true;
    }
    return false;
}

void ResetDoorLockSnapshot() {
    s_lockedAtLoadCount = 0;
    s_lockSnapshotArea  = nullptr;
}

bool IsDoorOpen(void* serverDoor) {
    if (!serverDoor) return false;
    __try {
        return *(reinterpret_cast<unsigned char*>(serverDoor) +
                 kDoorOpenStateOffset) != 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool IsDoorLocked(void* serverDoor) {
    if (!serverDoor ||
        GetObjectKind(serverDoor) != static_cast<int>(GameObjectKind::Door)) {
        return false;
    }
    uint32_t locked = 0;
    uint8_t  open   = 0;
    return ReadDoorFlags(serverDoor, locked, open) && locked != 0;
}

bool DoorHasTransitionDest(void* serverDoor) {
    if (!serverDoor ||
        GetObjectKind(serverDoor) != static_cast<int>(GameObjectKind::Door)) {
        return false;
    }
    char buf[128] = {0};
    return TryReadLocString(serverDoor, kDoorTransitionDestOffset, buf,
                            sizeof(buf)) &&
           buf[0];
}

bool IsDoorStatic(void* serverDoor) {
    if (!serverDoor) return false;
    __try {
        return *reinterpret_cast<uint32_t*>(
                   reinterpret_cast<unsigned char*>(serverDoor) +
                   kDoorStaticOffset) != 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool IsDoorCosmetic(void* serverDoor) {
    if (!IsDoorStatic(serverDoor)) return false;
    if (!acc::game::IsKotor2()) return true;
    // KOTOR 2 flags real, lock-bearing doors Static (see BuildDoorSuffix).
    // Any lock evidence — live lock, open state, or locked-at-load — means
    // it's a real door, not set dressing.
    uint32_t locked    = 0;
    uint8_t  openState = 0;
    ReadDoorFlags(serverDoor, locked, openState);
    return locked == 0 && openState == 0 &&
           !WasDoorLockedAtAreaLoad(serverDoor);
}

}  // namespace acc::engine

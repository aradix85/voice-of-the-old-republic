#include "door_announce.h"

#include <windows.h>
#include <cmath>
#include <cstdint>
#include <cstdio>

#include "camera_announce.h"
#include "engine_area.h"    // door position, base name, cosmetic test
#include "engine_panels.h"  // HasActiveDialogPanel
#include "engine_player.h"  // GetPlayerServerCreature
#include "engine_reads.h"
#include "filter_objects.h"
#include "interact_dispatch.h"  // walk-through: the open-door path of Enter
#include "log.h"
#include "engine_offsets_select.h"
#include "engine_game.h"
#include "speech.h"
#include "same_name_suffix.h"  // the door's number, as the cycles say it
#include "strings.h"

namespace acc::door_announce {

namespace {

// The autoturn that faced the door fires a direction-change announce a beat
// before the open; suppress a same-direction door readout inside this window
// so the two don't double up. (Per the design request: ~1s.)
constexpr unsigned int kDedupMs = 1000;

// CGameObject layout: vtable @0x0, id @0x4, object_type @0x8.
// Identical in both games (seeded kotor2_steam_aspyr.db) — CGameObject is the
// shallow root, and the KOTOR 2 insertions all happen further down the
// hierarchy, in CSWSObject and below.
const size_t kGameObjectIdOffset = acc::off::Same(0x4);

// One pending slot; the OpenDoor detour records into it, Tick drains it the
// same frame. Last-writer-wins if two doors open in one frame (negligible).
bool     s_pending       = false;
uint32_t s_pendingOpener = 0;
uint32_t s_pendingDoor   = 0;  // server handle, 0 when unknown (KOTOR 2)

// A door someone else opens (a story script, an enemy) is spoken when it is
// this close to the leader, once per door within the repeat window.
constexpr float kNearbyOpenM      = 20.0f;
constexpr DWORD kNearbyRepeatMs   = 3000;
uint32_t        s_lastNearbyDoor  = 0;
DWORD           s_lastNearbyAtMs  = 0;

// Walk-through after an Enter on a closed door into another area (see the
// header). The window is long because the engine first walks the leader to
// the door: 2026-11-09 the Upper City lift was 115 m away, the walk took 30 s
// and the lift opened the very tick a 30 s window ran out. The distance test
// keeps a door that opens after the player left from pulling them back. The
// short settle lets the open animation clear the doorway before the walk is
// queued.
constexpr DWORD kWalkThroughWindowMs = 120000;
constexpr DWORD kWalkThroughSettleMs = 600;
constexpr float kWalkThroughMaxM     = 6.0f;
uint32_t        s_walkDoor           = 0;   // server handle, 0 = disarmed
void*           s_walkArea           = nullptr;
DWORD           s_walkArmedAtMs      = 0;
DWORD           s_walkOpenSinceMs    = 0;

bool ReadServerObjectId(void* serverObj, uint32_t& outId) {
    return acc::engine::TryReadU32(serverObj, kGameObjectIdOffset, &outId);
}

// "Door 1 opens" for a door the leader did not open, near the leader. The
// Endar Spire story doors open from their own OnFailToOpen script after the
// player's attempt (opener = the door itself): without this line the last
// thing heard was "Door 1, locked", so a successful attempt sounded like a
// failed one (2026-11-08). Number without state words: at this point the
// lock flag may not have caught up with the open.
void AnnounceNearbyOpen(uint32_t doorHandle) {
    if (doorHandle == 0) return;
    void* door = acc::engine::ResolveServerObjectHandle(doorHandle);
    if (!door || acc::engine::IsDoorCosmetic(door)) return;
    if (acc::engine::HasActiveDialogPanel()) return;
    Vector dp;
    Vector pp;
    if (!acc::engine::GetObjectPosition(door, dp) ||
        !acc::engine::GetPlayerPosition(pp)) {
        return;
    }
    const float dist = DistXY(dp, pp);
    if (dist > kNearbyOpenM) return;
    const DWORD now = GetTickCount();
    if (doorHandle == s_lastNearbyDoor && now - s_lastNearbyAtMs < kNearbyRepeatMs) {
        return;
    }
    char name[160] = {0};
    if (!acc::engine::GetObjectBaseName(door, name, sizeof(name)) || !name[0]) {
        return;
    }
    acc::narration::AppendAreaPositionOrdinal(
        door, acc::filter::CycleCategory::Door, name, sizeof(name));
    char line[256];
    std::snprintf(line, sizeof(line),
                  acc::strings::Get(acc::strings::Id::FmtDoorOpensNearby), name);
    acc::speech::Speak(line, /*interrupt=*/false);
    s_lastNearbyDoor = doorHandle;
    s_lastNearbyAtMs = now;
    acclog::Write("DoorAnnounce", "nearby open 0x%08x at %.1f m -> [%s]",
                  doorHandle, dist, line);
}

void PollWalkThrough() {
    if (s_walkDoor == 0) return;
    const uint32_t h   = s_walkDoor;
    const DWORD    now = GetTickCount();
    if (now - s_walkArmedAtMs > kWalkThroughWindowMs) {
        s_walkDoor = 0;
        acclog::Write("DoorAnnounce", "walk-through 0x%08x: window passed, "
                      "door did not open", h);
        return;
    }
    if (acc::engine::GetCurrentArea() != s_walkArea) {
        s_walkDoor = 0;
        acclog::Write("DoorAnnounce", "walk-through 0x%08x: area changed, "
                      "dropped", h);
        return;
    }
    // A guard's conversation can come first (Upper City lift): wait it out.
    if (acc::engine::HasActiveDialogPanel()) return;
    void* door = acc::engine::ResolveServerObjectHandle(h);
    if (!door) {
        s_walkDoor = 0;
        return;
    }
    if (!acc::engine::IsDoorOpen(door)) {
        s_walkOpenSinceMs = 0;
        return;
    }
    if (s_walkOpenSinceMs == 0) {
        s_walkOpenSinceMs = now ? now : 1;
        return;
    }
    if (now - s_walkOpenSinceMs < kWalkThroughSettleMs) return;
    s_walkDoor = 0;
    Vector dp;
    Vector pp;
    if (!acc::engine::GetObjectPosition(door, dp) ||
        !acc::engine::GetPlayerPosition(pp)) {
        return;
    }
    const float dist = DistXY(dp, pp);
    if (dist > kWalkThroughMaxM) {
        acclog::Write("DoorAnnounce", "walk-through 0x%08x: open, but the "
                      "leader is %.1f m away; not walking", h, dist);
        return;
    }
    acclog::Write("DoorAnnounce", "walk-through 0x%08x: open, leader %.1f m "
                  "away; going through", h, dist);
    acc::interact::DispatchInteract(door, h, /*forceRadial=*/false);
}

}  // namespace

void ArmWalkThrough(uint32_t doorHandle) {
    // KOTOR 1 only: the door pointer resolve and open-state read below are
    // the same ones the nearby-open line uses, which is K1-only.
    if (acc::game::IsKotor2() || doorHandle == 0) return;
    s_walkDoor        = doorHandle & ~0x80000000u;
    s_walkArea        = acc::engine::GetCurrentArea();
    s_walkArmedAtMs   = GetTickCount();
    s_walkOpenSinceMs = 0;
    acclog::Write("DoorAnnounce", "walk-through armed for 0x%08x", s_walkDoor);
}

void DisarmWalkThrough() {
    if (s_walkDoor == 0) return;
    acclog::Write("DoorAnnounce", "walk-through 0x%08x disarmed by another "
                  "interact", s_walkDoor);
    s_walkDoor = 0;
}

void NoteDoorOpened(uint32_t openerServerId, uint32_t doorHandle) {
    s_pendingOpener = openerServerId;
    s_pendingDoor   = doorHandle;
    s_pending       = true;
}

void Tick() {
    PollWalkThrough();
    if (!s_pending) return;
    s_pending = false;
    uint32_t opener = s_pendingOpener;
    uint32_t door   = s_pendingDoor;

    // Only the player-controlled character's own door-opens re-orient the
    // player. GetPlayerServerCreature is the currently-controlled leader's
    // server creature; its game-object id shares the server namespace with the
    // opener id OpenDoor passed us.
    void*    leaderCreature = acc::engine::GetPlayerServerCreature();
    uint32_t leaderId       = 0;
    if (!ReadServerObjectId(leaderCreature, leaderId) || leaderId == 0) return;

    if (opener != leaderId) {
        acclog::Write("DoorAnnounce",
            "door opened by 0x%08x != leader 0x%08x — no facing readout",
            opener, leaderId);
        AnnounceNearbyOpen(door);
        return;
    }

    acc::camera_announce::AnnounceCurrentFacing(kDedupMs);
}

}  // namespace acc::door_announce

// CSWSDoor::OpenDoor detour @0x00589ceb — fires at the START of the open, right
// where the engine stamps the opener id into the door and is about to call
// SetOpenState to begin the animation. ESI = CSWSDoor* (this, unused here),
// EDI = param_1 = opener's SERVER object id. Server-side, so the id is directly
// comparable to the leader's. We only record; the facing readout + player
// filter + dedup run on the next tick.
extern "C" void __cdecl OnDoorOpen(void* serverDoor, uint32_t openerServerId) {
    // Gate cleared for KOTOR 2 in Batch 3. Whole chain K2-verified: the tick
    // reads the leader via GetPlayerServerObject (K2 branch calls the engine's
    // GetServerCreature) and the facing readout's camera fields are Pick'd.
    acc::door_announce::NoteDoorOpened(openerServerId,
                                       acc::engine::GetObjectHandle(serverDoor));
}

// CSWSDoor::OpenDoor @0x00619D00 on KOTOR 2, hooked at 0x00619DAA — right
// before the opener stamp `MOV [EAX+0x36C],ECX` (identity decompile-confirmed:
// stamp plus the open-direction dot test against the opener's position).
// Unoptimised frame: door `this` = [EBP-0x3C], opener id = [EBP+8].
extern "C" __declspec(dllexport)
void __cdecl OnDoorOpenK2(void* ebp) {
    if (!acc::game::IsKotor2()) return;
    if (!ebp) return;

    uint32_t openerId = 0;
    __try {
        openerId = *reinterpret_cast<uint32_t*>(static_cast<char*>(ebp) + 8);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return;
    }

    // The door pointer is not read here: the nearby-open line is KOTOR 1 only.
    acc::door_announce::NoteDoorOpened(openerId, 0);
}

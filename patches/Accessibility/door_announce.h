// Door-open facing readout.
//
// KOTOR autoturns the player to face whatever they interact with, so after
// opening a door the player has often been rotated (and walked) away from
// where they thought they were pointing. This announces the player's current
// camera facing the moment a door BEGINS to open, so the user re-establishes
// their bearing (and, implicitly, where they came from).
//
// Signal: the CSWSDoor::OpenDoor detour fires at the start of the open (before
// SetOpenState kicks off the animation), handing us the opener's server object
// id. Only doors the player-controlled leader opens re-read the facing — a
// follower / enemy / script opening a door nearby must not fire a readout.
// Those instead get one short line when the door is near the leader ("Door 1
// opens", KOTOR 1): a story door that opens from its own script after the
// player's attempt would otherwise go unannounced.
//
// De-noising lives in camera_announce::AnnounceCurrentFacing: the same sector
// spoken by the per-tick direction-change announce within the last second is
// suppressed, so a close door the player already got a direction cue for stays
// quiet, while a door reached after walking/turning speaks.

#pragma once

#include <cstdint>

namespace acc::door_announce {

// Called from the OpenDoor detour (main thread) with the opener's SERVER
// object id and the door's server handle (0 when unknown). Records them for
// the next Tick; no speech here.
void NoteDoorOpened(uint32_t openerServerId, uint32_t doorHandle);

// Per-tick consumer. Must run AFTER camera_announce::Tick so the dedup sees
// this frame's last-spoken sector. Cheap when idle (one flag check).
void Tick();

}  // namespace acc::door_announce

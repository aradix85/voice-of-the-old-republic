// What the minigame files share on the engine side: the area's minigame, the
// minigame object array, a track follower's rendered position and bounding
// boxes, and the offset field that steers the controlled object.
//
// Both the swoop bike and the turret gun are a CSWMiniPlayer (extends
// CSWTrackFollower). Each holds an `offset` Vector that
// CSWMiniPlayer::Control integrates every tick (`offset += axis-velocity`) and
// then applies to the controlled object:
//   - turret: offset.x = elevation, offset.z = azimuth, in degrees;
//   - swoop:  offset.x = the lateral lane coordinate (world-X tracks tunnel-X
//             1:1, so a unit of offset.x is a unit of world-X).
// Because the engine re-integrates offset from its current value each tick, a
// write issued after Control has run (from the OnUpdate tick, where the
// minigame modules live) sticks: the next tick only adds the player's own
// small delta on top. That is how the aim assists steer.

#pragma once

#include "engine_offsets.h"  // Vector
#include "engine_reads.h"
#include "minigame_latch.h"

namespace acc::minigame {

// Field reads against engine objects come from engine_reads: a stale or
// half-torn-down engine pointer reads as 0 or null.
using acc::engine::ReadFloatOr0;
using acc::engine::ReadPtrOrNull;
using acc::engine::ReadU32Or0;
using acc::engine::TryReadVector;

// CSWCArea.mini_game of the player's area, or nullptr. The truth at the
// moment of detection; a caller latches it, because this chain churns
// during the transition into and out of a minigame.
void* ReadAreaMiniGame();

// One tick of `latch` for the minigame of `type` (CSWMiniGame.type), with
// both truth sources read from the engine. A re-latch is logged under
// `logTag`.
LatchStep TickLatch(Latch& latch, uint32_t type, const char* logTag);

// The minigame object array (obstacles, accelerator pads, track followers),
// or nullptr when no minigame is live.
void* ResolveMgoArray();

// World position of `follower`'s first model: the model is what the engine
// renders and what the spatial cues must track, and the follower's own
// transform is not the same. False, with out untouched, when the follower,
// its model or the model's GetPosition is missing or faults.
bool ReadFollowerPosition(void* follower, Vector& out);

// `follower`'s first model object: a Gob (render object), the class the
// obstacles' CAurObject slot points at, so the two readers below apply to it
// (and GetModelName, which tells a KOTOR 2 accelerator pad from a mine). Null
// on any fault.
void* ReadFollowerModelGob(void* follower);

// Model-local bounding box of a Gob (whole model). False on fault / null.
bool ReadGobBoundingBox(void* gob, Vector& mn, Vector& mx);
// Model-local bounding box of one named part of a Gob. False when the part is
// missing, the box comes back empty, or anything faults.
bool ReadGobPartBoundingBox(void* gob, const char* partName, Vector& mn, Vector& mx);

// The whole offset Vector of a CSWMiniPlayer. Read is false, with out
// untouched, for a null or faulting player; write does nothing then. A caller
// that changes one component reads, changes and writes, to keep the others.
bool ReadOffsetVector(void* player, Vector& out);
void WriteOffsetVector(void* player, const Vector& v);

}  // namespace acc::minigame

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

#include <cstddef>
#include <cstdint>

#include "engine_offsets.h"  // Vector
#include "engine_reads.h"
#include "engine_offsets_select.h"

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

// ---- Minigame object array -------------------------------------------------
// CClientExoAppInternal holds the minigame object array (obstacles, accelerator
// pads, track followers) at offset 0. Resolution walks
// AppManager -> CClientExoApp -> internal -> array.
//
// KOTOR 2: identical. Witnessed in its GetMGOArray twin @0x00740070, which is
// literally `internal = [this+4]; return [internal+0]` — the same two hops.
const size_t kClientInternalMgoArrayOffset = acc::off::Same(0x0);

// Returns the minigame object array, or nullptr if any link of the chain is
// null or faults (i.e. no minigame is live).
void* ResolveMgoArray();

// Call a vtable AsSWCxxx-style downcast at `vtableSlotOffset` on `obj`.
// Returns nullptr on null input, a null slot, or any fault.
void* CallAsCast(void* obj, size_t vtableSlotOffset);

// ---- Track-follower world position -----------------------------------------
// A CSWTrackFollower (the swoop bike, the turret gun, and every racer /
// obstacle rider) carries a CExoArrayList of model wrappers at +0x68. The
// first model's vtable slot +0x64 is a GetPosition thunk that fills a
// caller-supplied Vector. Reading the follower's own transform is not
// equivalent — the model is what the engine actually renders and what the
// spatial cues must track.
//
// KOTOR 2: both identical, and the identification is exact — its
// CSWTrackFollower::GetPosition twin @0x00835f50 reads size at +0x6c, data at
// +0x68, takes element [0], and calls the model's vtable[+0x64] to copy three
// floats, instruction for instruction like KOTOR 1's @0x0066d5d0. (The list's
// element STRIDE is 8 bytes in BOTH games — the engine's own PlayAnimation walk
// indexes `data[i*8]` on each — so `data[0]` being the bare model pointer holds
// either way. Only index 0 is ever read here.)
const size_t kTrackFollowerModelsDataOffset = acc::off::Same(0x68);
constexpr size_t kModelVtableSlotGetPosition    = 0x64;

// World position of `follower`'s first model. False (out untouched) if the
// follower, its model list, the model, or the thunk is missing or faults.
bool ReadFollowerPosition(void* follower, Vector& out);

// `follower`'s first model object — a Gob (render object), the same class the
// obstacles' CAurObject slot points at, so the Gob accessors below apply to it
// (GetModelName tells a K2 accelerator pad from a mine). Null on any fault.
void* ReadFollowerModelGob(void* follower);

// Gob (render object) vtable slots, identical in both games — the RTTI slot map
// (docs/llm-docs/re/k2/k2-vtable-slots.csv, class "Gob") pairs them exactly:
//   slot  3 GetModelName()                       K1 0x0043F1C0 / K2 0x004615D0
//   slot 59 GetBoundingBox(Vector* mn, Vector* mx) K1 0x0043E8C0 / K2 0x00459FB0
//           (copies the member box — the model header's, MODEL-LOCAL, so the
//           world box is gob position + local box for an unrotated object)
//   slot 67 GetPart(const char* name) -> Part*   K1 0x004402F0 / K2 0x004587F0
//           ("root" is the root part; anything else is FindPart by name)
// Part vtable slot 29 GetMinimumBoundingBox(Vector* mn, Vector* mx, float scale)
//           K1 0x00440960 / K2 0x0045C810 — walks the part's trimesh vertices
//           and folds them into the CALLER-INITIALISED min/max, in the gob's
//           local frame (it subtracts the gob position and un-rotates).
// K1's CSWTrackFollower::DoObstacleHitCheck asks the obstacle's gob for its
// "hitbump" part and hit-tests against that (the string is literal in the
// decompile), so the part box is the true collision extent; the K2 swoop
// tracks' obstacle models carry exactly one such mesh (offline atlas,
// docs/llm-docs/swoop-k2-controls-and-jump.md section 7).
constexpr size_t kGobVtableSlotGetBoundingBox = 59 * sizeof(void*);
constexpr size_t kGobVtableSlotGetPart        = 67 * sizeof(void*);
constexpr size_t kPartVtableSlotGetMinBBox    = 29 * sizeof(void*);

// Model-local bounding box of a Gob (whole model). False on fault / null.
bool ReadGobBoundingBox(void* gob, Vector& mn, Vector& mx);
// Model-local bounding box of one named part of a Gob. False when the part is
// missing, the box comes back empty, or anything faults.
bool ReadGobPartBoundingBox(void* gob, const char* partName, Vector& mn, Vector& mx);

// Read / write the whole offset Vector on a CSWMiniPlayer `player` pointer.
// Read returns false (and leaves `out` untouched) on a null/faulting player;
// write is a no-op on null/fault. Callers that touch only one component should
// read-modify-write so the other components are preserved.
bool ReadOffsetVector(void* player, Vector& out);
void WriteOffsetVector(void* player, const Vector& v);

}  // namespace acc::minigame

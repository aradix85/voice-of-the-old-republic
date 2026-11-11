// Combat readouts over a creature's condition, effects and weapons.
// Read-only.
//
//   - The brief added to a creature's name when it becomes the target.
//   - H: the leader's own status.

#pragma once

#include "strings.h"

#include <cstddef>

namespace acc::combat::query {

// Writes "name, condition, distance, effects, weapons" for a creature target
// into outBuf. False, with outBuf empty, for any other kind of object.
bool BuildTargetCombatBrief(void* targetServerObject, const char* targetName,
                            char* outBuf, size_t outBufSize);

// H: speaks the leader's hit points, Force points, effects and weapons.
// Silent before the player is loaded and while a blocking panel is in front.
void PollWin32SelfStatusHotkey();

// Display name of the item in one inventory slot of a server creature.
// slotHandleOffset is one of the kInventory*HandleOffset constants; slotLabel
// names the slot in the log. False, with outBuf empty, for an empty slot or
// a name that does not resolve.
bool ReadEquippedItemName(void* serverCreature, size_t slotHandleOffset,
                          const char* slotLabel,
                          char* outBuf, size_t outBufSize);

// The string for ReadObjectDamageLevel's 0..5 bucket.
acc::strings::Id DamageLevelStringId(int level);

}  // namespace acc::combat::query

// Speaks the name of the creature the player controls when it changes: by
// Tab, in the world or in a panel, or by a script. A Tab that changes nobody
// answers with the current name, so the key never goes unanswered. Silent on
// the first observation and right after an area change, when the engine is
// putting the party together.

#pragma once

namespace acc::party_leader_announce {

void Tick();

}  // namespace acc::party_leader_announce

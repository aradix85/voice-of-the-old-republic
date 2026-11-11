# party_leader_announce.cpp (88 lines)

Source: `patches/Accessibility/party_leader_announce.cpp`

(From party_leader_announce.h.) Speaks the name of the creature the player controls when it changes: by Tab, in the world or in a panel, or by a script. A Tab that changes nobody answers with the current name, so the key never goes unanswered. Silent on the first observation and right after an area change, when the engine is putting the party together.

## Declarations (in source order)

- L37 `void Tick()`

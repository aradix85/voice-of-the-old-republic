# door_announce.cpp (245 lines)

Source: `patches/Accessibility/door_announce.cpp`

(From door_announce.h.) Door-open facing readout.

KOTOR autoturns the player to face whatever they interact with, so after opening a door the player has often been rotated (and walked) away from where they thought they were pointing. This announces the player's current camera facing the moment a door BEGINS to open, so the user re-establishes their bearing (and, implicitly, where they came from).

Signal: the CSWSDoor::OpenDoor detour fires at the start of the open (before SetOpenState kicks off the animation), handing us the opener's server object id. Only doors the player-controlled leader opens re-read the facing — a follower / enemy / script opening a door nearby must not fire a readout. Those instead get one short line when the door is near the leader ("Door 1 opens", KOTOR 1): a story door that opens from its own script after the player's attempt would otherwise go unannounced.

De-noising lives in camera_announce::AnnounceCurrentFacing: the same sector spoken by the per-tick direction-change announce within the last second is suppressed, so a close door the player already got a direction cue for stays quiet, while a door reached after walking/turning speaks.

## Declarations (in source order)

- L65 `bool ReadServerObjectId(void* serverObj, uint32_t& outId)`
- L75 `void AnnounceNearbyOpen(uint32_t doorHandle)`: "Door 1 opens" for a door the leader did not open, near the leader.
- L108 `void PollWalkThrough()`
- L160 `void ArmWalkThrough(uint32_t doorHandle)`
- L171 `void DisarmWalkThrough()`
- L178 `void NoteDoorOpened(uint32_t openerServerId, uint32_t doorHandle)`
- L184 `void Tick()`
- L218 `extern "C" void __cdecl OnDoorOpen(void* serverDoor, uint32_t openerServerId)`: CSWSDoor::OpenDoor detour @0x00589ceb — fires at the START of the open, right where the engine stamps the opener id into the door and is about to call SetOpenState to begin the animation.
- L231 `void __cdecl OnDoorOpenK2(void* ebp)`

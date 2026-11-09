# door_announce.h (48 lines)

Source: `patches/Accessibility/door_announce.h`

Door-open facing readout.

KOTOR autoturns the player to face whatever they interact with, so after opening a door the player has often been rotated (and walked) away from where they thought they were pointing. This announces the player's current camera facing the moment a door BEGINS to open, so the user re-establishes their bearing (and, implicitly, where they came from).

Signal: the CSWSDoor::OpenDoor detour fires at the start of the open (before SetOpenState kicks off the animation), handing us the opener's server object id. Only doors the player-controlled leader opens re-read the facing — a follower / enemy / script opening a door nearby must not fire a readout. Those instead get one short line when the door is near the leader ("Door 1 opens", KOTOR 1): a story door that opens from its own script after the player's attempt would otherwise go unannounced.

De-noising lives in camera_announce::AnnounceCurrentFacing: the same sector spoken by the per-tick direction-change announce within the last second is suppressed, so a close door the player already got a direction cue for stays quiet, while a door reached after walking/turning speaks.

## Declarations (in source order)

- L31 `void NoteDoorOpened(uint32_t openerServerId, uint32_t doorHandle)`: Called from the OpenDoor detour (main thread) with the opener's SERVER object id and the door's server handle (0 when unknown).
- L35 `void Tick()`: Per-tick consumer.
- L44 `void ArmWalkThrough(uint32_t doorHandle)`: Walk-through: one Enter takes the player through a closed door into another area.
- L45 `void DisarmWalkThrough()`

# announce_facing.cpp (217 lines)

Source: `patches/Accessibility/announce_facing.cpp`

(From announce_facing.h.) On-demand facing readout (AltGr / Right Alt).

In the world: the camera's facing as a compass direction word ("Facing north"), then the area the player stands in. On the map screen: the facing in map-frame degrees plus the direction word, then the room on a line of its own, so the heading never runs into the area description.

Camera (not character) because A/D rotates the camera and W only snaps the character on commit — the camera direction is what every other orient cue references, so a manual readout on a different frame would disagree with the last sector word the user heard.

Win32 polling because AltGr is unbound in stock kotor.ini and the engine keymap drops unbound scancodes before our manager hook sees them.

## Declarations (in source order)

- L26 `int CompassDegreesFromEngineYaw(float engineYaw)`
- L36 `bool ReadCameraEngineYawDegrees(float& out)`: Camera facing in engine yaw (0° = +X, CCW+).
- L51 `const char* SectorWord(int compassDegrees)`
- L60 `bool ResolveClusterLabelForPlayer(std::string& outBuf)`: Perceptual-region label at the player's position.
- L86 `void AnnounceWorldFacing()`
- L111 `bool ResolveRoomNameForPlayer(char* outBuf, size_t bufSize)`: Three-tier room name lookup matching transitions::Tick: landmark → friendly room name → synthetic "Room N".
- L150 `void AnnounceMapFacing()`
- L203 `void PollWin32()`

# guidance_description.h (32 lines)

Source: `patches/Accessibility/guidance_description.h`

Turn-by-turn TTS readout of a computed path.

Output shape: "Route to door: 5 metres north, 4 metres north-east, 6 metres east, no transition."

Each segment is the displacement between consecutive waypoints. Same- sector segments merge so a long corridor doesn't read as five "north". Sub-1m segments fold into the next (noise from "nearest path_point is 30cm from player" stays out).

Transition suffix: "1 transition" when the destination is a transition trigger, else "no transition". User knows their walk will cross-load.

## Declarations (in source order)

- L25 `bool Speak(const Vector& playerPos, const std::vector<Vector>& waypoints, const char* targetName, bool isTransition, bool interrupt)`: targetName is caller-resolved.

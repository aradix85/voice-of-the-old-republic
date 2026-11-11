# guidance_description.cpp (179 lines)

Source: `patches/Accessibility/guidance_description.cpp`

(From guidance_description.h.) Turn-by-turn TTS readout of a computed path.

Output shape: "Route to door: 5 metres north, 4 metres north-east, 6 metres east, no transition."

Each segment is the displacement between consecutive waypoints. Same- sector segments merge so a long corridor doesn't read as five "north". Sub-1m segments fold into the next (noise from "nearest path_point is 30cm from player" stays out).

Transition suffix: "1 transition" when the destination is a transition trigger, else "no transition". User knows their walk will cross-load.

## Declarations (in source order)

- L32 `void AddHop(float dx, float dy)`
- L62 `const std::vector<Segment>& Segments() const`
- L64 `float Pending() const`
- L73 `int FormatSegment(const Segment& seg, char* outBuf, size_t outBufSize)`: Format one segment as "{rounded metres} metres {DirectionWord}".
- L84 `int TotalMetres(const std::vector<Segment>& segs, float pending)`: Sum the rounded segment distances.
- L92 `bool Speak(const Vector& playerPos, const std::vector<Vector>& waypoints, const char* targetName, bool isTransition, bool interrupt)`

# room_topology_words.cpp (477 lines)

Source: `patches/Accessibility/room_topology_words.cpp`

The words a region label is made of: octant helpers, exit entries and the exit list, door and transition directions, the corridor axis, the far-end suffix and ramps.

## Declarations (in source order)

- L13 `void NoteUnspokenExit(const Vector& pos)`
- L23 `acc::strings::Id OctantFromVector(float dx, float dy)`: 8-way octant classifier.
- L50 `int OctantBit(acc::strings::Id dir)`: Map an Id::Dir* to a stable bit index 0..7 (for dedup masks).
- L67 `acc::strings::Id BitToOctant(int bit)`: Reverse of OctantBit: bit index → direction Id.
- L88 `std::string RenderTransitionExit(int trigIdx, const Vector& centroid)`: Spoken form of an attached transition trigger, e.g.
- L111 `void AppendListEntry(std::string& list, const std::string& entry)`: Append `entry` to a comma-separated list string, inserting ", " before it when the list is non-empty.
- L117 `void NoteExit(const std::string& entry, const Vector& pos)`
- L123 `Vector ExitPoint(int doorIdx, const Vector& fallback)`: Where an exit entry leads: the door's own position when it is a door, else `fallback` (the nav point the shape path measured it from).
- L147 `std::string RenderDoorDirection(int doorIdx, const char* dirWord)`: Render the door-flavoured replacement of a direction word.
- L227 `std::string RenderCorridorAxis(int bitA, int doorA, bool deadA, int bitB, int doorB, bool deadB, bool rampA, bool rampB, const Vector& posA, const Vector& posB)`: Render a corridor's axis label into outBuf.
- L337 `std::string DirEntry(acc::strings::Id dirId, bool markDeadEnd, bool markRamp)`: Render one direction entry.
- L421 `const char* FarEndSuffix(int bit, const Vector& exitPos, const float* bboxXY)`: Far-end exit qualifier (see the FarAt* string ids).
- L470 `bool ExitIsRamp(int nb, float clusterFloorZ)`: Does the exit toward external neighbour `nb` cross a floor-level change?

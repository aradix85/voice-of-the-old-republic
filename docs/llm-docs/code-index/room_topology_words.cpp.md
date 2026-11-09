# room_topology_words.cpp (476 lines)

Source: `patches/Accessibility/room_topology_words.cpp`

The words a region label is made of: octant helpers, exit entries and the exit list, door and transition directions, the corridor axis, the far-end suffix and ramps.

## Declarations (in source order)

- L11 `void NoteUnspokenExit(const Vector& pos)`
- L21 `acc::strings::Id OctantFromVector(float dx, float dy)`: 8-way octant classifier.
- L48 `int OctantBit(acc::strings::Id dir)`: Map an Id::Dir* to a stable bit index 0..7 (for dedup masks).
- L65 `acc::strings::Id BitToOctant(int bit)`: Reverse of OctantBit: bit index → direction Id.
- L86 `std::string RenderTransitionExit(int trigIdx, const Vector& centroid)`: Spoken form of an attached transition trigger, e.g.
- L109 `void AppendListEntry(std::string& list, const std::string& entry)`: Append `entry` to a comma-separated list string, inserting ", " before it when the list is non-empty.
- L115 `void NoteExit(const std::string& entry, const Vector& pos)`
- L121 `Vector ExitPoint(int doorIdx, const Vector& fallback)`: Where an exit entry leads: the door's own position when it is a door, else `fallback` (the nav point the shape path measured it from).
- L145 `std::string RenderDoorDirection(int doorIdx, const char* dirWord)`: Render the door-flavoured replacement of a direction word.
- L225 `std::string RenderCorridorAxis(int bitA, int doorA, bool deadA, int bitB, int doorB, bool deadB, bool rampA, bool rampB, const Vector& posA, const Vector& posB)`: Render a corridor's axis label into outBuf.
- L336 `std::string DirEntry(acc::strings::Id dirId, bool markDeadEnd, bool markRamp)`: Render one direction entry.
- L420 `const char* FarEndSuffix(int bit, const Vector& exitPos, const float* bboxXY)`: Far-end exit qualifier (see the FarAt* string ids).
- L469 `bool ExitIsRamp(int nb, float clusterFloorZ)`: Does the exit toward external neighbour `nb` cross a floor-level change?

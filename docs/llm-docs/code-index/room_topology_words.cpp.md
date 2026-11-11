# room_topology_words.cpp (457 lines)

Source: `patches/Accessibility/room_topology_words.cpp`

The words a region label is made of: octant helpers, exit entries and the exit list, door and transition directions, the corridor axis, the far-end suffix and ramps.

## Declarations (in source order)

- L14 `void NoteUnspokenExit(const Vector& pos)`
- L24 `acc::strings::Id OctantFromVector(float dx, float dy)`: 8-way octant classifier.
- L30 `int OctantBit(acc::strings::Id dir)`: Map an Id::Dir* to a stable bit index 0..7 (for dedup masks), counter-clockwise from east: E=0, NE=1, N=2, NW=3, W=4, SW=5, S=6, SE=7.
- L47 `acc::strings::Id BitToOctant(int bit)`: Reverse of OctantBit: bit index → direction Id.
- L68 `std::string RenderTransitionExit(int trigIdx, const Vector& centroid)`: Spoken form of an attached transition trigger, e.g.
- L91 `void AppendListEntry(std::string& list, const std::string& entry)`: Append `entry` to a comma-separated list string, inserting ", " before it when the list is non-empty.
- L97 `void NoteExit(const std::string& entry, const Vector& pos)`
- L103 `Vector ExitPoint(int doorIdx, const Vector& fallback)`: Where an exit entry leads: the door's own position when it is a door, else `fallback` (the nav point the shape path measured it from).
- L127 `std::string RenderDoorDirection(int doorIdx, const char* dirWord)`: Render the door-flavoured replacement of a direction word.
- L207 `std::string RenderCorridorAxis(int bitA, int doorA, bool deadA, int bitB, int doorB, bool deadB, bool rampA, bool rampB, const Vector& posA, const Vector& posB)`: Render a corridor's axis label into outBuf.
- L317 `std::string DirEntry(acc::strings::Id dirId, bool markDeadEnd, bool markRamp)`: Render one direction entry.
- L401 `const char* FarEndSuffix(int bit, const Vector& exitPos, const float* bboxXY)`: Far-end exit qualifier (see the FarAt* string ids).
- L450 `bool ExitIsRamp(int nb, float clusterFloorZ)`: Does the exit toward external neighbour `nb` cross a floor-level change?

# floor_puzzle.cpp (633 lines)

Source: `patches/Accessibility/floor_puzzle.cpp`

Rakatan temple floor-plate puzzle assist — see floor_puzzle.h for the puzzle model (decompiled k_punk_floor01..09 / k_punk_reset scripts).

## Declarations (in source order)

- L115 `void ResetAreaState()`
- L133 `const char* ShortName(int idx)`: Short name: the bare direction word ("Nord-West", "Mitte") used inside delta lists; kResetIndex gets the full reset-plate noun.
- L140 `std::string FullName(int idx)`: Full name: "Platte " for grid plates, the reset noun as-is.
- L147 `bool PointInPoly(const Plate& p, float x, float y)`
- L161 `float DistToSegment(float px, float py, const Vector& a, const Vector& b)`
- L175 `float DistToPlate(const Plate& p, float x, float y)`: 0 when inside; else the closest XY distance to any polygon edge.
- L197 `std::string AxisPhrase(const Vector& playerPos, const Plate& p)`: World-cardinal offsets to the plate's AREA, e.g.
- L226 `int TrigSlotForTag(const char* tag)`: Trigger tag -> slot.
- L235 `int PlcSlotForTag(const char* tag)`
- L243 `bool BuildCache(void* area)`
- L326 `void SpeakIntro(const Vector& playerPos)`
- L356 `std::string DeltaText(const bool* flipped, const bool* lit, int litCount)`: Itemized flip list, phrased as the TRANSITION each plate just made ("Nord-West leuchtet auf, West erlischt") + running lit count.
- L377 `std::string BoardStateText(const bool* lit, int litCount)`: Live board summary spoken on demand (R key).
- L396 `bool GetNavPosition(Vector& out)`: Board-navigation position: the view-mode virtual cursor when view mode is active, otherwise the real party leader.
- L407 `int ReadLiveBoard(bool* lit)`: Read the live lit/dark state off the plate placeables.
- L421 `bool IsActive()`
- L423 `bool IsPuzzlePlateTrigger(uint32_t handle)`
- L431 `void Tick()`

# floor_puzzle.cpp (658 lines)

Source: `patches/Accessibility/floor_puzzle.cpp`

Rakatan temple floor-plate puzzle assist — see floor_puzzle.h for the puzzle model (decompiled k_punk_floor01..09 / k_punk_reset scripts).

## Declarations (in source order)

- L115 `void ResetAreaState()`
- L133 `const char* ShortName(int idx)`: Short name: the bare direction word ("North-West", "Center") used inside delta lists; kResetIndex gets the full reset-plate noun.
- L140 `std::string FullName(int idx)`: Full name: "Plate " for grid plates, the reset noun as-is.
- L147 `bool PointInPoly(const Plate& p, float x, float y)`
- L161 `float DistToSegment(float px, float py, const Vector& a, const Vector& b)`
- L175 `float DistToPlate(const Plate& p, float x, float y)`: 0 when inside; else the closest XY distance to any polygon edge.
- L197 `std::string AxisPhrase(const Vector& playerPos, const Plate& p)`: World-cardinal offsets to the plate's AREA, e.g.
- L226 `int TrigSlotForTag(const char* tag)`: Trigger tag -> slot.
- L235 `int PlcSlotForTag(const char* tag)`
- L243 `bool BuildCache(void* area)`
- L326 `void SpeakIntro(const Vector& playerPos)`
- L356 `std::string DeltaText(const bool* flipped, const bool* lit, int litCount)`: Itemized flip list, phrased as the TRANSITION each plate just made ("North-West lights up, West goes dark") + running lit count.
- L377 `std::string BoardStateText(const bool* lit, int litCount)`: Live board summary spoken on demand (R key).
- L396 `bool GetNavPosition(Vector& out)`: Board-navigation position: the view-mode virtual cursor when view mode is active, otherwise the real party leader.
- L407 `int ReadLiveBoard(bool* lit)`: Read the live lit/dark state off the plate placeables.
- L429 `void MaybeReadBoard()`: On-demand board read (R) — every frame, ABOVE the scan throttle.
- L444 `bool TrackArea()`: Follows the current area and builds the plate cache inside the puzzle module.
- L474 `BoardScan ReadBoardScan()`
- L498 `bool QuietAfterSolve(const BoardScan& b)`: Speaks the solve once; true while the module stays quiet after it.
- L517 `NavScan TrackNavPlate(DWORD now)`: Leader while walking; the virtual cursor while view mode (B) is active, so the distance/entry stream follows what the player scans.
- L538 `std::string EntryText(int plate)`: The reset plate gets no explanatory sentence: its name plus the "all plates dark" delta that follows says everything.
- L544 `bool AnnounceEntryOrDelta(const BoardScan& b, const NavScan& n, DWORD now)`: Entry and toggle speech.
- L594 `void AnnounceNearest(const Vector& pos, DWORD now)`: Off-plate: nearest-plate stream.
- L626 `bool IsActive()`
- L628 `bool IsPuzzlePlateTrigger(uint32_t handle)`
- L636 `void Tick()`

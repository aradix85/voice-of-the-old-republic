# floor_puzzle.cpp (583 lines)

Source: `patches/Accessibility/floor_puzzle.cpp`

Rakatan temple floor-plate puzzle assist — see floor_puzzle.h for the puzzle model (decompiled k_punk_floor01..09 / k_punk_reset scripts).

## Declarations (in source order)

- L98 `void ResetAreaState()`
- L116 `const char* ShortName(int idx)`: Short name: the bare direction word ("North-West", "Center") used inside delta lists; kResetIndex gets the full reset-plate noun.
- L123 `std::string FullName(int idx)`: Full name: "Plate " for grid plates, the reset noun as-is.
- L141 `std::string AxisPhrase(const Vector& playerPos, const Plate& p)`: World-cardinal offsets to the plate's AREA, e.g.
- L168 `bool BuildCache(void* area)`
- L251 `void SpeakIntro(const Vector& playerPos)`
- L281 `std::string DeltaText(const bool* flipped, const bool* lit, int litCount)`: Itemized flip list, phrased as the TRANSITION each plate just made ("North-West lights up, West goes dark") + running lit count.
- L302 `std::string BoardStateText(const bool* lit, int litCount)`: Live board summary spoken on demand (R key).
- L321 `bool GetNavPosition(Vector& out)`: Board-navigation position: the view-mode virtual cursor when view mode is active, otherwise the real party leader.
- L332 `int ReadLiveBoard(bool* lit)`: Read the live lit/dark state off the plate placeables.
- L354 `void MaybeReadBoard()`: On-demand board read (R) — every frame, ABOVE the scan throttle.
- L369 `bool TrackArea()`: Follows the current area and builds the plate cache inside the puzzle module.
- L399 `BoardScan ReadBoardScan()`
- L423 `bool QuietAfterSolve(const BoardScan& b)`: Speaks the solve once; true while the module stays quiet after it.
- L442 `NavScan TrackNavPlate(DWORD now)`: Leader while walking; the virtual cursor while view mode (B) is active, so the distance/entry stream follows what the player scans.
- L463 `std::string EntryText(int plate)`: The reset plate gets no explanatory sentence: its name plus the "all plates dark" delta that follows says everything.
- L469 `bool AnnounceEntryOrDelta(const BoardScan& b, const NavScan& n, DWORD now)`: Entry and toggle speech.
- L519 `void AnnounceNearest(const Vector& pos, DWORD now)`: Off-plate: nearest-plate stream.
- L551 `bool IsActive()`
- L553 `bool IsPuzzlePlateTrigger(uint32_t handle)`
- L561 `void Tick()`

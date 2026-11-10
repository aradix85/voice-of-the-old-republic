# floor_puzzle.cpp (531 lines)

Source: `patches/Accessibility/floor_puzzle.cpp`

Rakatan temple floor-plate puzzle assist — see floor_puzzle.h for the puzzle model (decompiled k_punk_floor01..09 / k_punk_reset scripts).

## Declarations (in source order)

- L103 `const char* ShortName(int idx)`: Short name: the bare direction word ("North-West", "Center") used inside delta lists; kResetIndex gets the full reset-plate noun.
- L110 `std::string FullName(int idx)`: Full name: "Plate " for grid plates, the reset noun as-is.
- L121 `std::string AxisPhrase(const Vector& playerPos, const Plate& p)`: World-cardinal steps to the plate's AREA, e.g.
- L136 `std::string WithLitCount(std::string list, int litCount)`
- L143 `std::string DeltaText(const bool* flipped, const bool* lit, int litCount)`: Itemized flip list, phrased as the TRANSITION each plate just made ("North-West lights up, West goes dark") + running lit count.
- L158 `std::string BoardStateText(const bool* lit, int litCount)`: Live board summary spoken on demand (R key): every lit plate plus the count, so the player can re-orient at any point ("North-West, Center, South-East.
- L175 `void ToWorldIfLocal(void* obj, const char* tag, Plate& p)`: The GIT authors trigger polygons as position + local offsets and the engine translates them to world on load.
- L192 `bool AddTriggerPlate(void* obj, const char* tag)`: True when the trigger is a puzzle plate with usable geometry.
- L205 `bool AddPlatePlaceable(void* obj, const char* tag)`: True when the placeable is a plate's lit-state carrier.
- L212 `bool BuildCache(void* area)`
- L252 `int ReadLiveBoard(bool* lit)`: The live lit/dark state off the plate placeables.
- L269 `bool GetNavPosition(Vector& out)`: Board-navigation position: the view-mode virtual cursor when view mode is active, otherwise the party leader.
- L284 `void MaybeReadBoard()`: On-demand board read (R), every frame, ABOVE the scan throttle: the rising edge from Pressed() lives for one OnUpdate tick and the throttle skips most frames.
- L299 `bool TrackArea()`: Follows the current area and builds the plate cache inside the puzzle module.
- L327 `BoardScan ReadBoardScan()`
- L351 `bool QuietAfterSolve(const BoardScan& b)`: Speaks the solve once; true while the module stays quiet after it.
- L368 `NavScan TrackNavPlate(DWORD now)`
- L393 `bool MaybeSpeakIntro(const Vector& playerPos)`: One-shot orientation line near the grid, with the board-read key, the in-story clue and, when followers would toggle plates too, the solo-mode key.
- L424 `std::string EntryText(int plate)`: The reset plate gets no explanatory sentence: its name plus the "all plates dark" delta that follows says everything.
- L430 `bool AnnounceEntryOrDelta(const BoardScan& b, const NavScan& n, DWORD now)`: Entry and toggle speech.
- L479 `void AnnounceNearest(const Vector& pos, DWORD now)`: Off-plate stream: the nearest plate and the cardinal steps to it.
- L504 `bool IsActive()`
- L506 `bool IsPuzzlePlateTrigger(uint32_t handle)`
- L514 `void Tick()`

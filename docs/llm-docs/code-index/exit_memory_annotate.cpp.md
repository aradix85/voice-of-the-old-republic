# exit_memory_annotate.cpp (351 lines)

Source: `patches/Accessibility/exit_memory_annotate.cpp`

Region announcements and target lines: what lies behind each exit, visited or new, the way back, the "Here:" list, what is still behind a visited door, the current region on demand, and the way to a target.

## Declarations (in source order)

- L38 `bool IsPartyMember(void* obj, const uint32_t* party, int count)`
- L46 `std::string HereList(void* area, int cluster)`
- L72 `std::vector<RegionEntry> RegionEntries(void* area, int cluster, float radiusM, bool record)`
- L144 `std::string RegionLabelAt(void* area, const Vector& player)`: The player's region label, annotated like the walk-in announce when the committed region is the one underfoot.
- L184 `std::string BehindDoorText(void* area, int farId)`: What is still in a visited region behind a door ("Matrik; Footlocker"), or "nothing left".
- L210 `bool Annotate(void* area, const std::string& label, const std::vector<ExitMark>& exits, std::string& outLabel)`
- L307 `std::string CurrentRegionText()`
- L315 `std::string TargetWayInfo(void* obj, const Vector& pos)`

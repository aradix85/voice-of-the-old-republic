# exit_memory_area_doors.cpp (163 lines)

Source: `patches/Accessibility/exit_memory_area_doors.cpp`

Doors into other areas: which door led where (ACC_EXIT_<area>) and the map notes seen in each area (ACC_SUM_<area>), kept in the save, so two areas with the same name can be told apart.

## Declarations (in source order)

- L32 `std::string ExitVarName(const std::string& key)`
- L34 `std::string SumVarName(const std::string& key)`
- L37 `std::vector<Spot> ReadExits(const std::string& areaKey)`: The door spots of `areaKey` with the module each leads to.
- L49 `void RecordExit(const std::string& areaKey, const Vector& pos, const std::string& toKey)`: Remember that the door at `pos` in `areaKey` leads to module `toKey`, replacing whatever was remembered for that spot.
- L60 `std::string ExitTargetAt(const std::string& areaKey, const Vector& doorPos)`: The module remembered for the door spot nearest `doorPos`, empty if none.
- L68 `std::string CurrentAreaSummary(void* area)`: The map notes of the current area that are not a way out, as the cycles name them, joined: "Largo's Apartment, Yun's Apartment".
- L107 `std::string TransitionDoorMemory(const Vector& doorPos)`: For a door into another area: the map notes of the area behind it, as last seen there.
- L121 `void TickAreaMemory()`: Per tick: the player's spot (for the door left by), the arrival check that records which door joins which two areas, and this area's notes.

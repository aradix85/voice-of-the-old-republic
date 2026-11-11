# exit_memory_visits.cpp (181 lines)

Source: `patches/Accessibility/exit_memory_visits.cpp`

Visited regions: kept per area in the save (ACC_VISIT_<area>), the area-enter and region-commit edges, regions seen from nearby, and the per-frame Tick that reads the save once the player has settled.

## Declarations (in source order)

- L35 `void Persist()`
- L47 `void RecordVisit(int id)`
- L52 `void LoadVisited()`
- L89 `void MarkNearbySeen()`
- L118 `void OnAreaEntered(void* area)`
- L153 `void OnClusterCommitted(int from, int to)`
- L160 `void OnClusterSeen(int id)`
- L164 `void Tick()`

# exit_memory_visits.cpp (201 lines)

Source: `patches/Accessibility/exit_memory_visits.cpp`

Visited regions: kept per area in the save (ACC_VISIT_<area>), the area-enter and region-commit edges, regions seen from nearby, and the per-frame Tick that reads the save once the player has settled.

## Declarations (in source order)

- L40 `void Persist()`
- L53 `void RecordVisit(int id)`
- L58 `void LoadVisited()`
- L97 `void MarkNearbySeen()`
- L126 `void OnAreaEntered(void* area)`
- L162 `void OnClusterCommitted(int from, int to)`
- L169 `void OnClusterSeen(int id)`
- L173 `void Tick()`

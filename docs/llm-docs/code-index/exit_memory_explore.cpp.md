# exit_memory_explore.cpp (329 lines)

Source: `patches/Accessibility/exit_memory_explore.cpp`

Walking for the player: Ctrl+Shift+N, region by region to the nearest unexplored region (opening doors on the way), and Ctrl+L to the nearest loot.

## Declarations (in source order)

- L36 `void StopChain(const char* why)`
- L45 `void* ClosedDoorOnHop(const Graph& g, void* area, int hopId)`: The closed door on the link from the player's region to the region `hopId` next to it, or null.
- L58 `bool ChainStep()`: One hop of the explore walk: from the player's region to the next region on the open way to the goal — through the closed door on that link (walked to and opened like Shift+- on it) or to the next region's centre.
- L100 `void ActOnArrival()`: The walk of WalkThenInteract reached the target's region: act on the target as Enter on it does.
- L113 `void WalkTowardUnexplored()`
- L194 `void WalkToNearestLoot()`
- L212 `bool WalkThenInteract(void* target, uint32_t handle, const char* name)`
- L262 `void TickExploreChain()`
- L313 `void PollWalkHotkeys()`

# exit_memory_explore.cpp (339 lines)

Source: `patches/Accessibility/exit_memory_explore.cpp`

Walking for the player: Ctrl+Shift+N, region by region to the nearest unexplored region (opening doors on the way), and Ctrl+L to the nearest loot.

## Declarations (in source order)

- L36 `void StopChain(const char* why)`
- L45 `void* ClosedDoorBetween(const Graph& g, void* area, int a, int b)`: The closed door on the link between the neighbouring regions `a` and `b`, or null.
- L55 `bool ClosedDoorOnWay(const Graph& g, void* area, int t)`: Whether a closed door stands anywhere on the open walk to region index `t`.
- L68 `bool ChainStep()`: One hop of the explore walk: from the player's region to the next region on the open way to the goal — through the closed door on that link (walked to and opened like Shift+- on it) or to the next region's centre.
- L110 `void ActOnArrival()`: The walk of WalkThenInteract reached the target's region: act on the target as Enter on it does.
- L123 `void WalkTowardUnexplored()`
- L204 `void WalkToNearestLoot()`
- L222 `bool WalkThenInteract(void* target, uint32_t handle, const char* name)`
- L272 `void TickExploreChain()`
- L323 `void PollWalkHotkeys()`

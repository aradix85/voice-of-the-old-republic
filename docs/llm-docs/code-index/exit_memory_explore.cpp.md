# exit_memory_explore.cpp (377 lines)

Source: `patches/Accessibility/exit_memory_explore.cpp`

Walking for the player: Ctrl+Shift+N, region by region to the nearest unexplored region (opening doors on the way), and Ctrl+L to the nearest loot.

## Declarations (in source order)

- L40 `void StopChain(const char* why)`
- L49 `void* ClosedDoorBetween(const Graph& g, void* area, int a, int b)`: The closed door on the link between the neighbouring regions `a` and `b`, or null.
- L59 `bool ClosedDoorOnWay(const Graph& g, void* area, int t)`: Whether a closed door stands anywhere on the open walk to region index `t`.
- L72 `bool ChainStep()`: One hop of the explore walk: from the player's region to the next region on the open way to the goal — through the closed door on that link (walked to and opened like Enter on it) or to the next region's walk point.
- L121 `void ActOnArrival()`: The walk of WalkThenInteract reached the target's region: act on the target as Enter on it does.
- L139 `void WalkTowardUnexplored()`
- L220 `void WalkToNearestLoot()`
- L241 `bool WalkThenAct(void* target, uint32_t handle, const char* name, bool refused, bool walkOnly)`: The region walk to a target in another region, and on arrival the request again: act on it, or with `walkOnly` walk up to it.
- L301 `bool WalkThenInteract(void* target, uint32_t handle, const char* name)`
- L305 `bool RescueWalk(void* target, const char* name, bool act)`
- L310 `void TickExploreChain()`
- L361 `void PollWalkHotkeys()`

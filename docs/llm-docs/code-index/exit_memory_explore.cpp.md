# exit_memory_explore.cpp (248 lines)

Source: `patches/Accessibility/exit_memory_explore.cpp`

Walking for the player: Ctrl+Shift+N, region by region to the nearest unexplored region (opening doors on the way), and Ctrl+L to the nearest loot.

## Declarations (in source order)

- L30 `void StopChain(const char* why)`
- L41 `bool ChainStep()`: One hop of the explore walk: from the player's region to the next region on the open way to the goal — through the closed door on that link (walked to and opened like Shift+- on it) or to the next region's centre.
- L88 `void WalkTowardUnexplored()`
- L169 `void WalkToNearestLoot()`
- L186 `void TickExploreChain()`
- L232 `void PollWalkHotkeys()`

# exit_memory_overview.cpp (461 lines)

Source: `patches/Accessibility/exit_memory_overview.cpp`

The exploration overview (Ctrl+N, the bridge's /overview), loot still to search, "No way there now", and the loot reminder on leaving a region.

## Declarations (in source order)

- L53 `void CollectLoot(std::vector<LootSpot>& out)`: Loot left: containers holding items, and items on the ground, nearest first (straight line).
- L97 `void FlushReminder()`
- L120 `bool CollectOverview(Overview& o)`
- L284 `std::string OverviewText(const Overview& o)`
- L361 `void PollOverviewHotkey()`
- L383 `void WarnIfBehindLock(void* target)`
- L405 `void RemindLootOnLeaving(void* target)`

# exit_memory_overview.cpp (456 lines)

Source: `patches/Accessibility/exit_memory_overview.cpp`

The exploration overview (Ctrl+N, the bridge's /overview), loot still to search, "No way there now", and the loot reminder on leaving a region.

## Declarations (in source order)

- L54 `void CollectLoot(std::vector<LootSpot>& out)`: Loot left: containers holding items, nearest first (straight line).
- L92 `void FlushReminder()`
- L115 `bool CollectOverview(Overview& o)`
- L279 `std::string OverviewText(const Overview& o)`
- L356 `void PollOverviewHotkey()`
- L378 `void WarnIfBehindLock(void* target)`
- L400 `void RemindLootOnLeaving(void* target)`

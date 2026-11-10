# exit_memory_overview.cpp (465 lines)

Source: `patches/Accessibility/exit_memory_overview.cpp`

The exploration overview (Ctrl+N, the bridge's /overview), loot still to search, "No way there now", and the loot reminder on leaving a region.

## Declarations (in source order)

- L53 `void CollectLoot(std::vector<LootSpot>& out)`: Loot left: containers holding items, and items on the ground, nearest first (straight line).
- L97 `void FlushReminder()`
- L120 `bool CollectOverview(Overview& o)`
- L282 `std::string OverviewText(const Overview& o)`
- L359 `void PollOverviewHotkey()`
- L381 `void WarnIfBehindLock(void* target)`
- L409 `void RemindLootOnLeaving(void* target)`

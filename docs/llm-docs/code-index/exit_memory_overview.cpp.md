# exit_memory_overview.cpp (466 lines)

Source: `patches/Accessibility/exit_memory_overview.cpp`

The exploration overview (Ctrl+N, the bridge's /overview), loot still to search, "No way there now", and the loot reminder on leaving a region.

## Declarations (in source order)

- L54 `void CollectLoot(std::vector<LootSpot>& out)`: Loot left: containers holding items, and items on the ground, nearest first (straight line).
- L98 `void FlushReminder()`
- L121 `bool CollectOverview(Overview& o)`
- L283 `std::string OverviewText(const Overview& o)`
- L360 `void PollOverviewHotkey()`
- L382 `void WarnIfBehindLock(void* target)`
- L410 `void RemindLootOnLeaving(void* target)`

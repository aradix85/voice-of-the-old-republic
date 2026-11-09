# cycle_state.cpp (480 lines)

Source: `patches/Accessibility/cycle_state.cpp`

cursor-anchored while view mode is on

## Declarations (in source order)

- L25 `void SortByDistanceAscending(CategoryListing& l)`: Insertion sort by distance ascending.
- L55 `void KeepWhatIsLeft(CategoryListing& l, acc::filter::CycleContext ctx)`: World cycles list what the player can still go to: nothing that lies only behind locked doors (or where the region graph has no way to; a door counts by the side the player walks up to), and no container already emptied.
- L80 `CycleState& GetState(acc::filter::CycleContext ctx)`
- L94 `bool BuildCategoryListing(acc::filter::CycleCategory category, CategoryListing& out, acc::filter::CycleContext ctx)`
- L353 `void* CycleNextItem(const CategoryListing& listing, acc::filter::CycleContext ctx)`
- L369 `void* CyclePrevItem(const CategoryListing& listing, acc::filter::CycleContext ctx)`
- L384 `void* CycleFirstItem(const CategoryListing& listing, acc::filter::CycleContext ctx)`
- L397 `void* CycleLastItem(const CategoryListing& listing, acc::filter::CycleContext ctx)`
- L413 `bool CycleCategoryDirectional(CategoryListing& outListing, bool forward, acc::filter::CycleContext ctx)`
- L440 `bool CycleNextCategory(CategoryListing& outListing, acc::filter::CycleContext ctx)`
- L445 `bool CyclePrevCategory(CategoryListing& outListing, acc::filter::CycleContext ctx)`
- L450 `bool RefreshCurrentListing(CategoryListing& outListing, acc::filter::CycleContext ctx)`

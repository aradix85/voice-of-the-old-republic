# cycle_state.cpp (476 lines)

Source: `patches/Accessibility/cycle_state.cpp`

(From cycle_state.h.) Pillar 4 cycle state — current category + focused-object tracking.

Single source of truth for "what's currently in the cycle". Input, announce, and guidance all go through this surface.

Cycle scope: whole-area listings sorted by distance. .lyt-rooms over-segment K1 into spatially-meaningless slivers, so they're not used for narrowing.

## Declarations (in source order)

- L23 `void SortByDistanceAscending(CategoryListing& l)`: Insertion sort by distance ascending.
- L53 `void KeepWhatIsLeft(CategoryListing& l, acc::filter::CycleContext ctx)`: World cycles list what the player can still go to: nothing that lies only behind locked doors (or where the region graph has no way to; a door counts by the side the player walks up to), and no container already empti...
- L78 `CycleState& GetState(acc::filter::CycleContext ctx)`
- L92 `bool BuildCategoryListing(acc::filter::CycleCategory category, CategoryListing& out, acc::filter::CycleContext ctx)`
- L349 `void* CycleNextItem(const CategoryListing& listing, acc::filter::CycleContext ctx)`
- L365 `void* CyclePrevItem(const CategoryListing& listing, acc::filter::CycleContext ctx)`
- L380 `void* CycleFirstItem(const CategoryListing& listing, acc::filter::CycleContext ctx)`
- L393 `void* CycleLastItem(const CategoryListing& listing, acc::filter::CycleContext ctx)`
- L409 `bool CycleCategoryDirectional(CategoryListing& outListing, bool forward, acc::filter::CycleContext ctx)`
- L436 `bool CycleNextCategory(CategoryListing& outListing, acc::filter::CycleContext ctx)`
- L441 `bool CyclePrevCategory(CategoryListing& outListing, acc::filter::CycleContext ctx)`
- L446 `bool RefreshCurrentListing(CategoryListing& outListing, acc::filter::CycleContext ctx)`

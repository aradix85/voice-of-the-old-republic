# cycle_state.h (96 lines)

Source: `patches/Accessibility/cycle_state.h`

Pillar 4 cycle state — current category + focused-object tracking.

Single source of truth for "what's currently in the cycle". Input, announce, and guidance all go through this surface.

Cycle scope: whole-area listings sorted by distance. .lyt-rooms over-segment K1 into spatially-meaningless slivers, so they're not used for narrowing.

## Declarations (in source order)

- L27 `struct CategoryListing`: Per-category listing.
- L42 `struct CycleState`: World and Map keep independent focus.
- L48 `CycleState& GetState(acc::filter::CycleContext ctx = acc::filter::CycleContext::World)`
- L57 `bool BuildCategoryListing(acc::filter::CycleCategory category, CategoryListing& out, acc::filter::CycleContext ctx = acc::filter::CycleContext::World)`: Build sorted-by-distance listing for `category`.
- L63 `void* CycleNextItem(const CategoryListing& listing, acc::filter::CycleContext ctx = acc::filter::CycleContext::World)`: Boundaries clamp (no wrap).
- L66 `void* CyclePrevItem(const CategoryListing& listing, acc::filter::CycleContext ctx = acc::filter::CycleContext::World)`
- L72 `void* CycleFirstItem(const CategoryListing& listing, acc::filter::CycleContext ctx = acc::filter::CycleContext::World)`: Jump straight to the first (closest) / last (farthest) item of the listing.
- L75 `void* CycleLastItem(const CategoryListing& listing, acc::filter::CycleContext ctx = acc::filter::CycleContext::World)`
- L82 `bool CycleNextCategory(CategoryListing& outListing, acc::filter::CycleContext ctx = acc::filter::CycleContext::World)`: Empty-category silent skip: scans up to 6 sibling categories until a non-empty one is found.
- L85 `bool CyclePrevCategory(CategoryListing& outListing, acc::filter::CycleContext ctx = acc::filter::CycleContext::World)`
- L91 `bool RefreshCurrentListing(CategoryListing& outListing, acc::filter::CycleContext ctx = acc::filter::CycleContext::World)`: Re-find previously-focused object in the new sorted order; if it's gone, reset to closest.

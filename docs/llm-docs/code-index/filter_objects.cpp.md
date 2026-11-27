# filter_objects.cpp (120 lines)

Source: `patches/Accessibility/filter_objects.cpp`

(From filter_objects.h.) Pillar 4 object filter — six locked categories.

Pure predicate over engine_area's GameObjectKind + sub-state. Sub-state filters: Container needs usable=true OR has_inventory=true; Landmark needs has_map_note=true; Transition needs transition_destination set.

## Declarations (in source order)

- L9 `const char* CategoryName(CycleCategory c)`
- L23 `acc::strings::Id CategoryNameId(CycleCategory c)`
- L38 `CycleCategory ClassifyObject(void* gameObject)`
- L46 `bool ObjectMatches(void* gameObject, CycleCategory category)`
- L95 `CycleCategory NextCategory(CycleCategory c)`
- L100 `CycleCategory PrevCategory(CycleCategory c)`
- L106 `bool IsMapCycleable(CycleCategory c)`

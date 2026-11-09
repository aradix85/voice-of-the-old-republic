# filter_objects.cpp (105 lines)

Source: `patches/Accessibility/filter_objects.cpp`

(From filter_objects.h.) Pillar 4 object filter — six locked categories.

Pure predicate over engine_area's GameObjectKind + sub-state. Sub-state filters: Container needs usable=true OR has_inventory=true; Landmark needs has_map_note=true; Transition needs transition_destination set.

## Declarations (in source order)

- L8 `const char* CategoryName(CycleCategory c)`
- L21 `acc::strings::Id CategoryNameId(CycleCategory c)`
- L35 `bool ObjectMatches(void* gameObject, CycleCategory category)`
- L80 `CycleCategory NextCategory(CycleCategory c)`
- L85 `CycleCategory PrevCategory(CycleCategory c)`
- L91 `bool IsMapCycleable(CycleCategory c)`

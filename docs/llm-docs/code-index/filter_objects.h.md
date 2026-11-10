# filter_objects.h (56 lines)

Source: `patches/Accessibility/filter_objects.h`

Pillar 4 object filter — six locked categories.

Pure predicate over engine_area's GameObjectKind + sub-state. Sub-state filters: Container needs usable=true OR has_inventory=true; Landmark needs has_map_note=true; Transition needs transition_destination set.

## Declarations (in source order)

- L15 `enum class CycleCategory`: Cycle order returned by NextCategory / PrevCategory.
- L27 `const char* CategoryName(CycleCategory c)`: English, for logs.
- L30 `acc::strings::Id CategoryNameId(CycleCategory c)`: Localised, for speech: the same mapping as CategoryName.
- L33 `bool ObjectMatches(void* gameObject, CycleCategory category)`: SEH-guarded via engine_area::GetObjectKind.
- L37 `CycleCategory ClassifyObject(void* gameObject)`: The first category the object matches, in enum order; Count_ for none (a combat or dialog target, say).
- L39 `CycleCategory NextCategory(CycleCategory c)`
- L40 `CycleCategory PrevCategory(CycleCategory c)`
- L44 `enum class CycleContext`: World = in-world cycle over CSWSArea object list; Map = same data projected onto the area map, fog-of-war-gated.
- L53 `bool IsMapCycleable(CycleCategory c)`: Which categories the engine renders as map icons.

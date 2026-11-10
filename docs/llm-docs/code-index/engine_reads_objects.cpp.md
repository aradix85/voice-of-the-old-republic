# engine_reads_objects.cpp (90 lines)

Source: `patches/Accessibility/engine_reads_objects.cpp`

Hit points, damage level and plot flag of server objects.

## Declarations (in source order)

- L28 `int ReadObjectDamageLevel(void* serverObject)`: KOTOR 1: CSWSObject::GetDamageLevel @0x004cb020.
- L56 `int ReadCreatureCurrentHitPoints(void* serverCreature)`
- L67 `int ReadCreatureMaxHitPoints(void* serverCreature)`
- L78 `int ReadObjectCurrentHitPoints(void* serverObject)`
- L84 `bool IsPlotObject(void* serverObject)`

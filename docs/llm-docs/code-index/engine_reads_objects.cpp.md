# engine_reads_objects.cpp (111 lines)

Source: `patches/Accessibility/engine_reads_objects.cpp`

Hit points, damage level and plot flag of server objects.

## Declarations (in source order)

- L28 `int ReadObjectDamageLevel(void* serverObject)`: KOTOR 1: CSWSObject::GetDamageLevel @0x004cb020.
- L66 `int ReadCreatureCurrentHitPoints(void* serverCreature)`
- L77 `int ReadCreatureMaxHitPoints(void* serverCreature)`
- L88 `int ReadObjectCurrentHitPoints(void* serverObject)`
- L99 `bool IsPlotObject(void* serverObject)`

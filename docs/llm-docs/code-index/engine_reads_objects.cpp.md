# engine_reads_objects.cpp (188 lines)

Source: `patches/Accessibility/engine_reads_objects.cpp`

Hit points, Force points, effects, damage level and plot flag of creatures and objects.

## Declarations (in source order)

- L26 `void* ClientCreatureStats(void* clientCreature)`: The CSWCCreatureStats block of a client creature; null on fault.
- L34 `int ReadU16OfEach(void* data, int count, size_t fieldOffset, int* out)`: The u16 at fieldOffset of each non-null element of a pointer array.
- L59 `int ReadObjectDamageLevel(void* serverObject)`: KOTOR 1: CSWSObject::GetDamageLevel @0x004cb020.
- L87 `int ReadCreatureCurrentHitPoints(void* serverCreature)`
- L98 `int ReadCreatureMaxHitPoints(void* serverCreature)`
- L109 `int ReadObjectCurrentHitPoints(void* serverObject)`
- L115 `bool ReadClientCreatureHitPoints(void* clientCreature, int* outCur, int* outMax)`
- L128 `bool ReadCreatureForcePoints(void* clientCreature, int* outCur, int* outMax)`
- L158 `int ReadCreatureEffectIconIds(void* serverCreature, int* outIds, int cap)`
- L169 `int ReadObjectEffectTypes(void* serverObject, int* outTypes, int cap)`
- L182 `bool IsPlotObject(void* serverObject)`

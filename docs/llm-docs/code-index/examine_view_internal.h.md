# examine_view_internal.h (50 lines)

Source: `patches/Accessibility/examine_view_internal.h`

Internal to the examine_view module (examine_view*.cpp); no other file includes it. The public interface is examine_view.h. Here: what the files share.

## Declarations (in source order)

- L19 `struct State`
- L34 `bool IsSentinel(uint32_t handle)`: examine_view_reads.cpp
- L35 `void* ReadCreatureStats(void* serverCreature)`
- L36 `int ReadHpMax(void* serverCreature)`
- L37 `int ReadLevel(void* serverCreature)`
- L38 `int ReadDamageLevel(void* obj)`
- L39 `bool ReadBlindFlag(void* serverCreature)`
- L40 `acc::strings::Id DamageLevelStringId(int level)`
- L41 `int ReadFactionId(void* serverCreature)`
- L42 `acc::strings::Id FactionWordIdFor(int factionId)`
- L43 `int Read2DDistanceMeters(void* obj)`
- L44 `uint32_t ReadLastTargetHandle()`
- L47 `int BuildRows()`: examine_view_rows.cpp

Also 2 constants or variables at namespace scope.

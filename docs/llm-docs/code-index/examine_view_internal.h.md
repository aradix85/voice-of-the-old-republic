# examine_view_internal.h (49 lines)

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
- L40 `int ReadFactionId(void* serverCreature)`
- L41 `acc::strings::Id FactionWordIdFor(int factionId)`
- L42 `int Read2DDistanceMeters(void* obj)`
- L43 `uint32_t ReadLastTargetHandle()`
- L46 `int BuildRows()`: examine_view_rows.cpp

Also 2 constants or variables at namespace scope.

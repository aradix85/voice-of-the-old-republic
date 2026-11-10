# examine_view_internal.h (54 lines)

Source: `patches/Accessibility/examine_view_internal.h`

Internal to the examine_view module (examine_view*.cpp); no other file includes it. The public interface is examine_view.h. Here: what the files share.

## Declarations (in source order)

- L22 `struct State`
- L36 `bool IsSentinel(uint32_t handle)`: examine_view_reads.cpp
- L37 `void* ReadCreatureStats(void* serverCreature)`
- L38 `int ReadHpMax(void* serverCreature)`
- L39 `int ReadLevel(void* serverCreature)`
- L40 `int ReadDamageLevel(void* obj)`
- L41 `bool ReadBlindFlag(void* serverCreature)`
- L42 `acc::strings::Id DamageLevelStringId(int level)`
- L43 `int ReadFactionId(void* serverCreature)`
- L44 `acc::strings::Id FactionWordIdFor(int factionId)`
- L45 `int Read2DDistanceMeters(void* obj)`
- L46 `bool ReadEquippedItemNameAtSlot(void* serverCreature, size_t slotOffset, char* outBuf, size_t outBufSize)`
- L48 `uint32_t ReadLastTargetHandle()`
- L51 `int BuildRows()`: examine_view_rows.cpp

Also 2 constants or variables at namespace scope.

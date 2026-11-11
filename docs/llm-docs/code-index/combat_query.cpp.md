# combat_query.cpp (264 lines)

Source: `patches/Accessibility/combat_query.cpp`

Combat readouts: the brief added to a creature's name when it becomes the target, and the leader's own status on H.

## Declarations (in source order)

- L24 `bool ReadEquippedItemName(void* serverCreature, size_t slotHandleOffset, const char* slotLabel, char* outBuf, size_t outBufSize)`
- L69 `float PlayerDistanceMeters(void* target)`: Horizontal distance from the player to the target, -1 when a position is unreadable.
- L81 `bool JoinEffectNames(const int* ids, int count, const char* (*nameOf)(int), char* outBuf, size_t outBufSize)`: Comma-joins the names of up to kMaxEffectsSpoken distinct ids; an id without a name is skipped.
- L108 `bool BuildEffectsSummary(void* serverCreature, char* outBuf, size_t outBufSize)`: The effects clause.
- L127 `void AppendLoadout(acc::strfmt::FixedText& b, void* serverCreature, Loadout& out)`: What both readouts end with: the active effects, then what each hand holds.
- L144 `void SpeakSelfStatus()`: The leader's hit points, Force points for a Force user, effects and weapons.
- L192 `acc::strings::Id DamageLevelStringId(int level)`
- L201 `bool BuildTargetCombatBrief(void* targetServerObject, const char* targetName, char* outBuf, size_t outBufSize)`
- L240 `void PollWin32SelfStatusHotkey()`

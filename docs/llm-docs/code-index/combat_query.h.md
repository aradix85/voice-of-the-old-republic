# combat_query.h (36 lines)

Source: `patches/Accessibility/combat_query.h`

Combat readouts over a creature's condition, effects and weapons. Read-only.

- The brief added to a creature's name when it becomes the target. - H: the leader's own status.

## Declarations (in source order)

- L17 `bool BuildTargetCombatBrief(void* targetServerObject, const char* targetName, char* outBuf, size_t outBufSize)`: Writes "name, condition, distance, effects, weapons" for a creature target into outBuf.
- L22 `void PollWin32SelfStatusHotkey()`: H: speaks the leader's hit points, Force points, effects and weapons.
- L28 `bool ReadEquippedItemName(void* serverCreature, size_t slotHandleOffset, const char* slotLabel, char* outBuf, size_t outBufSize)`: Display name of the item in one inventory slot of a server creature.
- L33 `acc::strings::Id DamageLevelStringId(int level)`: The string for ReadObjectDamageLevel's 0..5 bucket.

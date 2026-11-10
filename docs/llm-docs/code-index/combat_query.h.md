# combat_query.h (39 lines)

Source: `patches/Accessibility/combat_query.h`

Combat read-side queries. Read-only over creature_stats @+0xa74; no engine re-entry beyond documented accessors. Each entry self-gates on player loaded.

Surfaces: - Cycle/passive-narrate enrichment for Creature-kind targets. - Bare H self-status (HP + effects + equipped weapons).

## Declarations (in source order)

- L19 `bool BuildTargetCombatBrief(void* targetServerObject, const char* targetName, char* outBuf, size_t outBufSize)`: Append HP/AC/faction suffix to outBuf.
- L25 `void SpeakSelfStatus()`: Bare H — HP + active effects (deduped, named) + equipped weapons.
- L27 `void PollWin32SelfStatusHotkey()`
- L34 `bool ReadEquippedItemName(void* serverCreature, size_t slotHandleOffset, const char* slotLabel, char* outBuf, size_t outBufSize)`: Resolve the display name of the item equipped in one inventory slot (creature → CSWInventory @+kCreatureInventoryOffset → slot handle → display name; SEH-guarded per hop).

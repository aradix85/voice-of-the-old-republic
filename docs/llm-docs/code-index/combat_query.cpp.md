# combat_query.cpp (587 lines)

Source: `patches/Accessibility/combat_query.cpp`

transient (engine LYT loader use-after-free)

## Declarations (in source order)

- L61 `int ReadCurrentHpFromClient(void* clientLeader)`
- L81 `int ReadMaxHpFromClient(void* clientLeader)`
- L112 `bool ReadEquippedItemName(void* serverCreature, size_t slotHandleOffset, const char* slotLabel, char* outBuf, size_t outBufSize)`: Walk CSWSCreature.inventory @+0xa2c → CSWInventory.
- L168 `float ComputePlayerDistanceMeters(void* targetObject)`: 2D distance (horizontal plane) from player to target in metres.
- L186 `void BriefAppend(BriefBuf& b, const char* fmt, ...)`
- L202 `int BuildEffectIconSummary(void* serverObject, char* outBuf, size_t outBufSize)`: Read the creature's effect-icon row (CSWSCreature.effect_icons — the exact icon list the sighted portrait renders; see engine_offsets.h) and join the localized icon names.
- L254 `bool BuildEffectsSummary(void* serverObject, char* outBuf, size_t outBufSize)`: Produce a comma-joined effects string for the brief / self status.
- L303 `acc::strings::Id DamageLevelStringIdFor(int level)`
- L318 `void TickLeaderChangeAutoAnnounce()`
- L395 `bool BuildTargetCombatBrief(void* targetServerObject, const char* targetName, char* outBuf, size_t outBufSize)`
- L485 `void SpeakSelfStatus()`
- L560 `void PollWin32SelfStatusHotkey()`

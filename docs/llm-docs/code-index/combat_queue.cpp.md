# combat_queue.cpp (873 lines)

Source: `patches/Accessibility/combat_queue.cpp`

GetObjectDisplayNameByHandle

## Declarations (in source order)

- L45 `void* ReadCombatRound(void* serverCreature)`: Read CSWSCreature.combat_round @+0x9c8.
- L69 `bool ReadNodeActionType(void* node, unsigned char& outType)`
- L84 `int CountQueueEntries(void* combatRound)`
- L120 `void* GetQueueAction(void* combatRound, int index)`: Get the action node at zero-based `index` (skipping placeholder type=0xFF entries), or nullptr if out of range.
- L177 `acc::strings::Id VerbForActionType(unsigned char actionType)`: Map an action_type byte to a localised verb.
- L190 `bool ReadActionFields(void* action, unsigned char& outType, uint32_t& outTarget)`: Read action_type byte + target handle from a CSWSCombatRoundAction.
- L212 `unsigned short ReadActionFeatId(void* action)`: Read the feat_id stored on a type=11 (Use Feat) action node.
- L226 `int ReadActionSpellId(void* action)`: Read the spell_id stored on a type=9 (Cast Force Power) action node.
- L240 `uint32_t ReadActionItemHandle(void* action)`: Read the item handle stored on a type=10 (Use Item) action node.
- L290 `void ResolveMemberName(uint32_t handle, char* outBuf, size_t bufSize)`: Resolve the display name for a companion party member by handle.
- L302 `int BuildRows()`: Rebuild g_state.rows from live engine state.
- L382 `void BuildActionLabel(void* action, char* out, size_t n)`: Build the spoken label for a queued action node: the SPECIFIC ability name for force powers / items / feats (type 9/10/11), else the generic verb ("Angriff" / "Talent einsetzen" / ...).
- L424 `int RawRoundCount(void* combatRound)`: Read the raw queue count from a combat round (engine's own actions->internal->count — the value AddAction's `if (3 < count)` cap inspects).
- L446 `void SpeakRow(int idx)`: Speak the focused row at index `idx` (0-based) of `count` total.
- L496 `bool RemoveRow(int idx)`: Try to remove the action at row `idx`.
- L523 `bool ClearAllRows()`: Clear every party member's queue via repeated RemoveLastAction calls against each row's combat round.
- L545 `int CountPlayerEntries()`
- L604 `void ArmUserQueueAdd()`
- L606 `void ArmNoOpWatch()`
- L608 `void CancelNoOpWatch()`
- L610 `void TickNoOpWatch()`
- L621 `void OnEngineActionAdded(void* combatRound, void* action)`
- L682 `void ReportPrePressDepth()`
- L686 `int GetPrePressDepth()`
- L697 `bool IsActive()`
- L699 `void ForceDisarm(const char* reason)`
- L709 `bool Open()`
- L733 `void ClearAllAndClose()`
- L750 `bool HandleInputEvent(int code, int value)`
- L848 `void Tick()`
- L862 `void PollWin32Hotkey()`

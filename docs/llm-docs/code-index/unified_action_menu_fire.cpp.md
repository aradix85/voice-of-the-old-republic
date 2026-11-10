# unified_action_menu_fire.cpp (302 lines)

Source: `patches/Accessibility/unified_action_menu_fire.cpp`

Firing an action from the unified action menu: the dispatch (queued or replacing), the engine's refusal, and whether the menu closes afterwards.

## Declarations (in source order)

- L30 `void RestampTargetRowForDispatch(void* tam, const Cat& cur)`: DoTargetAction dispatches at the creature_id baked into the matched list descriptor at PopulateMenus time — and while the menu sits open the engine re-bakes the lists against its own current target (combat reassert / ...
- L51 `uint32_t PendingRefusalStrRef(void* mi, const Cat& cur, int sel, bool* outRefused)`: Speak the engine's own refusal, when it is about to refuse.
- L67 `void SpeakRefusal(uint32_t strref)`: interrupt=false throughout: the bare-key path speaks the entry's label immediately before firing, and "Medikit" followed by the reason is the whole sentence the user needs — cutting the label off to deliver the reason...
- L95 `bool DispatchWithQueueAppend(void* tam, void* mi, const Cat& cur)`: Force the engine's APPEND path.
- L141 `void ApplyPostFireClosePolicy()`: What happens to the menu after a successful fire.
- L180 `bool HandleEnter(void* tam, void* mi, const Cat& cur, int sel, bool targetChanged, bool selectionCarried)`: Follow-cycling contract: Enter right after cycling fires immediately when the user's selected action exists on the new target (selection carried by action_id above).
- L258 `bool FirePersonal(int col)`: Fire a personal column's selected entry outright, with no menu.

# unified_action_menu_fire.cpp (312 lines)

Source: `patches/Accessibility/unified_action_menu_fire.cpp`

Firing an action from the unified action menu: the dispatch (queued or replacing), the engine's refusal, and whether the menu closes afterwards.

## Declarations (in source order)

- L31 `void RestampTargetRowForDispatch(void* tam, const Cat& cur)`: DoTargetAction dispatches at the creature_id baked into the matched list descriptor at PopulateMenus time — and while the menu sits open the engine re-bakes the lists against its own current target (combat reassert / ...
- L61 `uint32_t PendingRefusalStrRef(void* mi, const Cat& cur, int sel, bool* outRefused)`: Force the engine's APPEND path.
- L77 `void SpeakRefusal(uint32_t strref)`: interrupt=false throughout: the bare-key path speaks the entry's label immediately before firing, and "Medikit" followed by the reason is the whole sentence the user needs — cutting the label off to deliver the reason...
- L97 `bool DispatchWithQueueAppend(void* tam, void* mi, const Cat& cur)`
- L143 `void ApplyPostFireClosePolicy()`: What happens to the menu after a successful fire.
- L182 `bool HandleEnter(void* tam, void* mi, const Cat& cur, int sel, bool targetChanged, bool selectionCarried)`: Follow-cycling contract: Enter right after cycling fires immediately when the user's selected action exists on the new target (selection carried by action_id above).
- L260 `bool FirePersonal(int col)`: Fire a personal column's selected entry outright, with no menu.

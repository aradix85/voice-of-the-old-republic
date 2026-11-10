# unified_action_menu_open.cpp (369 lines)

Source: `patches/Accessibility/unified_action_menu_open.cpp`

Opening and closing the unified action menu, and keeping it on the right target while it is open.

## Declarations (in source order)

- L15 `bool ArmFromRadial(const char* name, uint32_t targetHandle)`
- L55 `bool OpenTarget(int row)`
- L122 `bool OpenPersonal(int col)`
- L228 `void CloseFromEsc()`: Esc — close.
- L264 `bool ReanchorToNewTarget(void* tam, uint32_t narrated, bool rowCarried[kRowCount])`: FOLLOW-CYCLING re-anchor: when the narrated target changed while the menu sat open (`,`/`.`/Q/E/passive all stamp the same slot), rebuild the target rows against the NEW target so the menu always shows that target's r...
- L309 `bool UnfoldTargetBlock(void* tam, uint32_t narrated, bool rowCarried[kRowCount])`: UNFOLD: the menu is personal-only (opened without a target, or its target was lost) and the user cycled onto something.
- L352 `void MaybeRestoreDrainedRows(void*& tam, bool targetChanged)`: LAZY re-anchor: re-anchoring on EVERY keypress broke target-menu navigation (the engine's per-press re-derivation churned the rows so arrows produced nothing).

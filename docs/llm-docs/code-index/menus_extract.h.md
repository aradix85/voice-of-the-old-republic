# menus_extract.h (76 lines)

Source: `patches/Accessibility/menus_extract.h`

control text extraction (the announce ladder).

of menus.cpp into its own TU as `acc::menus::extract::FromControl`. Function body is unchanged; the rename matches the namespace context (`acc::menus::extract::FromControl(ctrl, buf, n, owner)` reads as a typed call site rather than the bare verb-and-noun original name).

The cycle-category cache (used by the cycle-button announce path to recover the category prefix that the engine overwrites with the value on each activation) lives in this TU but is populated by OnSetActiveControl in menus.cpp on panel-walk. Hence the public setter pair below — read happens internally in FromControl, write crosses the seam.

Helpers used only by FromControl (FindSiblingLabel, IsCycleFlankerArrow, LookupCycleCategory) move with it as file-statics in menus_extract.cpp. Helpers shared with chain navigation (IsChainNavigable, IsClassSelection- Icon, ClassLabelCache*) stay in menus.cpp and cross via menus_internal.h.

## Declarations (in source order)

- L40 `const char* FromControl(void* control, char* outBuf, size_t bufSize, void* ownerPanel = nullptr)`: Pull the announceable text of a control (tooltip → button → label → labelhilight → slider → listbox → vtable-overrides → per-kind fallbacks → sibling label → cycle category prefix → toggle state suffix).
- L54 `void ResetCycleCategoryCache()`: Cycle-category cache management.
- L55 `void CaptureCycleCategory(void* control, const char* category)`
- L63 `bool ReadEquipSlotItemName(void* panel, void* slotBtn, char* outBuf, size_t bufSize)`: Resolve the equipped item's display name for one equip-screen slot button (matched by its .gui id against the slot table the per-kind announce uses).
- L70 `void ForEachWagerRowAnchor(void* panel, bool (*callback)(void* labelControl, int sortCy, void* userData), void* userData)`: Pazaak wager popup: register a single virtual top-of-chain "row" (live wager + maximum + credits) anchored on the maximum_label.

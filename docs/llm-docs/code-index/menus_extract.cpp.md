# menus_extract.cpp (2503 lines)

Source: `patches/Accessibility/menus_extract.cpp`

control text extraction (the announce ladder).

FromControl and its three extract-only helpers (FindSiblingLabel, IsCycleFlankerArrow, LookupCycleCategory) lifted out of menus.cpp. Helpers shared with chain navigation (IsChainNavigable, IsClassSelectionIcon, ClassLabelCache*, GetControlCenter) and the focused-panel pointer (g_currentPanel) cross the seam via menus_internal.h.

FromControl itself is the ladder: it does no extraction of its own, it runs the Try* steps in order and applies the three announce suffixes. Each step's rationale lives at that step's definition in the anonymous namespace below, under the number it had when the ladder was one function.

## Declarations (in source order)

- L126 `int PortraitVariantRank(uint32_t row)`: 1-based position of `row` inside its own (gender, family) group, or 0 when the row isn't one this game's table knows.
- L157 `bool EmitText(char* outBuf, size_t bufSize, const char* text)`: Hand `text` back to the caller, TRUNCATING when it does not fit.
- L184 `const char* LookupCycleCategory(void* control)`
- L213 `bool IsCycleFlankerArrow(void* panel, void* control)`: True for image-only buttons that flank a cycle value-display button at the same y-row (e.g.
- L286 `const char* FindSiblingLabel(void* panel, void* control, char* outBuf, size_t bufSize)`: Find the closest label-like sibling of `control` on the panel.
- L403 `bool IsSoundOptionsMovieSlider(void* panel, void* control)`: Identify the movie-volume slider on optionssound.gui.
- L446 `void* FindWagerMaxLabel(void* panel)`: LBL_MAXIMUM / BTN_LESS / BTN_MORE all resolve through the popup's own ctor-bound members (WagerPopupPanel* in menus_internal.cpp); the two games' .gui ids for them survive only as those resolvers' GuiIdMismatch tripwi...
- L452 `bool ExtractWagerRow(void* panel, void* maxLabel, char* outBuf, size_t bufSize)`: Format "Einsatz N.
- L517 `void* ResolveOwnerPanel(void* control, void* ownerPanel)`: Resolve the panel that owns `control`.
- L526 `const char* TryCharsheetStatRow(void* control, void* owner, char* outBuf, size_t bufSize)`: Character-sheet stat rows on InGameCharacter.
- L549 `const char* TryCreditsRow(void* control, void* owner, char* outBuf, size_t bufSize)`: Credits row (Inventory + Store).
- L571 `const char* TryEquipStatRow(void* control, void* owner, char* outBuf, size_t bufSize)`: Equip stat rows (Vitality, Defense, Attack, Damage).
- L593 `const char* TryPazaakDeckCard(void* control, void* owner, char* outBuf, size_t bufSize)`: Pazaak side-deck builder card widgets (CSWGuiPazaakStart).
- L615 `const char* TryPazaakWager(void* control, void* owner, char* outBuf, size_t bufSize)`: Pazaak wager popup (CSWGuiWagerPopup).
- L644 `const char* TryJournalQuestItemsButton(void* control, void* owner, char* outBuf, size_t bufSize)`: Journal quest-items button.
- L666 `const char* TryKeyBindingRow(void* control, void* owner, char* outBuf, size_t bufSize)`: Keyboard-mapping row (CSWGuiKeyMapButton).
- L720 `const char* TryVirtualRowAnchor(void* control, void* ownerPanel, char* outBuf, size_t bufSize)`: 0.
- L754 `const char* TryTooltip(void* control, char* outBuf, size_t bufSize)`: 1.
- L778 `const char* TryButton(void* control, char* outBuf, size_t bufSize)`: 2.
- L794 `const char* TryButtonToggle(void* control, char* outBuf, size_t bufSize)`: 3.
- L809 `const char* TryLabel(void* control, char* outBuf, size_t bufSize)`: 4.
- L824 `const char* TryLabelHilight(void* control, char* outBuf, size_t bufSize)`: 5.
- L862 `const char* TrySlider(void* control, void* owner, char* outBuf, size_t bufSize)`: 6.
- L904 `const char* TryEditbox(void* control, char* outBuf, size_t bufSize)`: 6b.
- L949 `const char* TrySingleRowListBox(void* control, char* outBuf, size_t bufSize)`: 7.
- L1034 `const char* TryPartyPortrait(void* control, char* outBuf, size_t bufSize, bool& claimedVtable)`: 7b.
- L1132 `const char* TrySpeculativeVtableRead(void* control, char* outBuf, size_t bufSize)`: 8.
- L1295 `const char* TryInGameMenuIcon(void* control, void* owner, char* outBuf, size_t bufSize)`: 9a.
- L1464 `const char* TryEquipSlot(void* control, void* owner, char* outBuf, size_t bufSize)`
- L1575 `const char* TryInGameMapArrow(void* control, void* owner, char* outBuf, size_t bufSize)`: 9b2.
- L1649 `const char* TryWorkbenchSlot(void* control, void* owner, char* outBuf, size_t bufSize)`
- L1795 `const char* TryWorkbenchCreateItemsButton(void* control, void* owner, char* outBuf, size_t bufSize)`: 9b4.
- L1862 `const char* TryClassSelectionIcon(void* control, void* owner, char* outBuf, size_t bufSize)`: 9c.
- L1956 `const char* TryPortraitCharGenArrow(void* control, void* owner, char* outBuf, size_t bufSize)`: 9d.
- L2150 `bool IsPanelDirectChild(void* panel, void* control)`: True when `control` sits directly in panel.controls[].
- L2178 `const char* TryItemEntryRow(void* control, char* outBuf, size_t bufSize)`: 9.
- L2232 `const char* TrySiblingLabel(void* control, void* owner, char* outBuf, size_t bufSize)`: 10.
- L2271 `void ApplyCycleCategoryPrefix(void* control, char* outBuf, size_t bufSize)`: Cycle value-display prefix.
- L2283 `void AppendToggleSuffix(void* control, char* outBuf, size_t bufSize)`: Append element-state suffix for toggles.
- L2320 `void AppendDisabledSuffix(void* control, void* ownerPanel, char* outBuf, size_t bufSize)`: Append "nicht verfügbar" / "unavailable" suffix when the focused button is engine-disabled.
- L2372 `void ResetCycleCategoryCache()`
- L2376 `void CaptureCycleCategory(void* control, const char* category)`
- L2397 `const char* FromControl(void* control, char* outBuf, size_t bufSize, void* ownerPanel)`
- L2467 `bool ReadEquipSlotItemName(void* panel, void* slotBtn, char* outBuf, size_t bufSize)`
- L2489 `void ForEachWagerRowAnchor(void* panel, bool (*callback)(void* labelControl, int sortCy, void* userData), void* userData)`

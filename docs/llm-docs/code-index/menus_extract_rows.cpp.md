# menus_extract_rows.cpp (264 lines)

Source: `patches/Accessibility/menus_extract_rows.cpp`

Rows that only exist in the mod's model of a screen (virtual rows): the character sheet stats, credits, equipment stats, pazaak deck and wager, the journal's quest-items button and key-binding rows.

## Declarations (in source order)

- L35 `void* FindWagerMaxLabel(void* panel)`: LBL_MAXIMUM / BTN_LESS / BTN_MORE all resolve through the popup's own ctor-bound members (WagerPopupPanel* in menus_internal.cpp); the two games' .gui ids for them survive only as those resolvers' GuiIdMismatch tripwi...
- L41 `bool ExtractWagerRow(void* panel, void* maxLabel, char* outBuf, size_t bufSize)`: Format "Wager N.
- L78 `const char* GuardedRow(bool (*extract)(void*, void*, char*, size_t), void* owner, void* control, char* outBuf, size_t bufSize, const char* tag)`: One SEH-guarded call of a screen model's row extractor: `tag` (the source named in the log) when it produced text, else null.
- L89 `const char* TryCharsheetStatRow(void* control, void* owner, char* outBuf, size_t bufSize)`: Character-sheet stat rows on InGameCharacter.
- L103 `const char* TryCreditsRow(void* control, void* owner, char* outBuf, size_t bufSize)`: Credits row (Inventory + Store).
- L116 `const char* TryEquipStatRow(void* control, void* owner, char* outBuf, size_t bufSize)`: Equip stat rows (Vitality, Defense, Attack, Damage).
- L129 `const char* TryPazaakDeckCard(void* control, void* owner, char* outBuf, size_t bufSize)`: Pazaak side-deck builder card widgets (CSWGuiPazaakStart).
- L140 `const char* TryPazaakWager(void* control, void* owner, char* outBuf, size_t bufSize)`: Pazaak wager popup (CSWGuiWagerPopup).
- L169 `const char* TryJournalQuestItemsButton(void* control, void* owner, char* outBuf, size_t bufSize)`: Journal quest-items button.
- L191 `const char* TryKeyBindingRow(void* control, void* owner, char* outBuf, size_t bufSize)`: Keyboard-mapping row (CSWGuiKeyMapButton).
- L237 `const char* TryVirtualRowAnchor(void* control, void* owner, char* outBuf, size_t bufSize)`: Per-kind row formatter for virtual chain entries.
- L250 `void ForEachWagerRowAnchor(void* panel, bool (*callback)(void* labelControl, int sortCy, void* userData), void* userData)`

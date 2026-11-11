# menus_journal.cpp (210 lines)

Source: `patches/Accessibility/menus_journal.cpp`

(From menus_journal.h.) journal panel (CSWGuiInGameJournal) helpers.

The journal lists active/done quest titles in items_listbox at +0x5c4. For sighted players, hovering a row triggers the engine's CSWGuiInGameJournal::OnControlEntered which rewrites item_description_label (at +0x1a4) with the full entry text (planet name + description). Pressing Enter on a row does nothing meaningful — the row's class fires no listener.

For screen-reader users we repurpose Enter on a journal row: it refreshes item_description_label for the focused entry and speaks the full text with interrupt. The arrow-key chain step still announces just the title, and Shift+Down/Up via peek_description still walks description blocks (the journal only has one block, so that path effectively replays the same text — kept consistent with other panels).

## Declarations (in source order)

- L23 `bool ReadRowText(void* row, char* outBuf, std::size_t bufSize)`
- L45 `bool IsJournalEntry(void* control)`
- L49 `void SpeakDescription(void* panel, void* focusedRow)`
- L118 `bool IsSortButton(void* panel, void* control)`
- L130 `bool IsSwapButton(void* panel, void* control)`
- L134 `void LogEntryCounts(void* panel)`
- L186 `void ForceRepopulate(void* panel)`

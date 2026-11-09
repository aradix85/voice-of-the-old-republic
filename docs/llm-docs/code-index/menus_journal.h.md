# menus_journal.h (57 lines)

Source: `patches/Accessibility/menus_journal.h`

journal panel (CSWGuiInGameJournal) helpers.

The journal lists active/done quest titles in items_listbox at +0x5c4. For sighted players, hovering a row triggers the engine's CSWGuiInGameJournal::OnControlEntered which rewrites item_description_label (at +0x1a4) with the full entry text (planet name + description). Pressing Enter on a row does nothing meaningful — the row's class fires no listener.

For screen-reader users we repurpose Enter on a journal row: it refreshes item_description_label for the focused entry and speaks the full text with interrupt. The arrow-key chain step still announces just the title, and Shift+Down/Up via peek_description still walks description blocks (the journal only has one block, so that path effectively replays the same text — kept consistent with other panels).

## Declarations (in source order)

- L24 `bool IsJournalEntry(void* control)`: True iff `control`'s vtable matches CSWGuiJournalItemEntry — a row of the items_listbox in CSWGuiInGameJournal.
- L33 `void SpeakDescription(void* panel, void* focusedRow)`: Refresh item_description_label for `focusedRow` (calls the engine's CSWGuiInGameJournal::OnControlEntered with the focused row) and speak the resulting description text with interrupt=true.
- L39 `bool IsSortButton(void* panel, void* control)`: True iff `control` is the journal's Sort button (panel+0xc2c).
- L44 `bool IsSwapButton(void* panel, void* control)`: True iff `control` is the journal's Swap button (panel+0xa68).
- L49 `void LogEntryCounts(void* panel)`: Diagnostic: log the raw CSWCJournal active/done entry counts (read directly from engine data, independent of the current view) plus the live items_listbox row count, so we can confirm the display surfaces every entry ...
- L54 `void ForceRepopulate(void* panel)`: Force a synchronous rebuild of the quest list by calling the engine's CSWGuiInGameJournal::PopulateItemListBox.

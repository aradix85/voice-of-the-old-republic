# menus_listbox_saveload.cpp (366 lines)

Source: `patches/Accessibility/menus_listbox_saveload.cpp`

The save and load screens: the save list, Enter and Escape, and on KOTOR 2 the selection monitor, the save information and the character swap.

## Declarations (in source order)

- L46 `bool SaveLoadMatches(void* p)`: K1 only, deliberately.
- L50 `void* SaveLoadFindLb(void* p)`
- L63 `int NewestPlayMinutes(void* lb, int rowCount)`: Speak on every step (no per-tick monitor watches the SaveLoad listbox).
- L77 `void SaveLoadAnnounce(void* lb, const ListBoxNavResult& r)`: The list puts the quicksave on top whether or not it is the latest, so the row with the longest play time says "newest": the save to load after a death is not always the first one.
- L107 `void SaveLoadLogExtra(char* out, size_t outN, const ListBoxNavResult& r)`
- L116 `bool SaveLoadOnEnter(void* panel)`
- L137 `bool SaveLoadOnEsc(void* panel)`
- L179 `void MonitorK2SaveLoadSelection()`: KOTOR 2 save/load: tell the engine which row the keyboard is on.
- L238 `void AnnounceK2SaveLoadInfo()`: KOTOR 2 save/load: the per-slot location, one tick after the sync.
- L291 `bool ReadK2SaveLoadSwapState(void* panel, void* lb, int* outIdx, int* outRows)`: One SEH block for every raw engine read this monitor needs.
- L313 `void MonitorK2SaveLoadCharacterSwap()`

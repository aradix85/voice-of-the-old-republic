# menus_listbox_saveload.cpp (343 lines)

Source: `patches/Accessibility/menus_listbox_saveload.cpp`

The save and load screens: the save list, Enter and Escape, and on KOTOR 2 the selection monitor, the save information and the character swap.

## Declarations (in source order)

- L43 `bool SaveLoadMatches(void* p)`: K1 only, deliberately.
- L47 `void* SaveLoadFindLb(void* p)`
- L58 `void SaveLoadAnnounce(void* /*lb*/, const ListBoxNavResult& r)`: Speak on every step (no per-tick monitor watches the SaveLoad listbox).
- L83 `void SaveLoadLogExtra(char* out, size_t outN, const ListBoxNavResult& r)`
- L92 `bool SaveLoadOnEnter(void* panel)`
- L113 `bool SaveLoadOnEsc(void* panel)`
- L155 `void MonitorK2SaveLoadSelection()`: KOTOR 2 save/load: tell the engine which row the keyboard is on.
- L214 `void AnnounceK2SaveLoadInfo()`: KOTOR 2 save/load: the per-slot location, one tick after the sync.
- L267 `bool ReadK2SaveLoadSwapState(void* panel, void* lb, int* outIdx, int* outRows)`: One SEH block for every raw engine read this monitor needs.
- L290 `void MonitorK2SaveLoadCharacterSwap()`

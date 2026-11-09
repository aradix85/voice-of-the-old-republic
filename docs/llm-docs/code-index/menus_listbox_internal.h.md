# menus_listbox_internal.h (217 lines)

Source: `patches/Accessibility/menus_listbox_internal.h`

Internal to the menus_listbox module (menus_listbox*.cpp); no other file includes it. The public interface is menus_listbox.h. Here: what the files share.

## Declarations (in source order)

- L69 `struct ListBoxPanelSpec`: Spec struct.
- L203 `void SpeakRowAndPosition(const ListBoxNavResult& r)`: menus_listbox.cpp
- L204 `int FindListBoxRowIndex(void* listbox, void* control)`
- L205 `bool SetListBoxSelectedControl(void* listbox, int index)`
- L208 `void MonitorContainerSelection()`: menus_listbox_container.cpp
- L209 `void PollContainerGiveModeKey()`
- L212 `void MonitorK2SaveLoadSelection()`: menus_listbox_saveload.cpp
- L213 `void AnnounceK2SaveLoadInfo()`
- L214 `void MonitorK2SaveLoadCharacterSwap()`

Also 35 constants or variables at namespace scope.

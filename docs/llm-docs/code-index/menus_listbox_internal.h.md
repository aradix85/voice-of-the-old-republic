# menus_listbox_internal.h (201 lines)

Source: `patches/Accessibility/menus_listbox_internal.h`

Internal to the menus_listbox module (menus_listbox*.cpp); no other file includes it. The public interface is menus_listbox.h. Here: what the files share.

## Declarations (in source order)

- L53 `struct ListBoxPanelSpec`: Spec struct.
- L187 `void SpeakRowAndPosition(const ListBoxNavResult& r)`: menus_listbox.cpp
- L188 `int FindListBoxRowIndex(void* listbox, void* control)`
- L189 `bool SetListBoxSelectedControl(void* listbox, int index)`
- L192 `void MonitorContainerSelection()`: menus_listbox_container.cpp
- L193 `void PollContainerGiveModeKey()`
- L196 `void MonitorK2SaveLoadSelection()`: menus_listbox_saveload.cpp
- L197 `void AnnounceK2SaveLoadInfo()`
- L198 `void MonitorK2SaveLoadCharacterSwap()`

Also 35 constants or variables at namespace scope.

# menus_listbox_internal.h (200 lines)

Source: `patches/Accessibility/menus_listbox_internal.h`

Internal to the menus_listbox module (menus_listbox*.cpp); no other file includes it. The public interface is menus_listbox.h. Here: what the files share.

## Declarations (in source order)

- L53 `struct ListBoxPanelSpec`: Spec struct.
- L187 `void SpeakRowAndPosition(const ListBoxNavResult& r)`: menus_listbox.cpp
- L188 `bool SetListBoxSelectedControl(void* listbox, int index)`
- L191 `void MonitorContainerSelection()`: menus_listbox_container.cpp
- L192 `void PollContainerGiveModeKey()`
- L195 `void MonitorK2SaveLoadSelection()`: menus_listbox_saveload.cpp
- L196 `void AnnounceK2SaveLoadInfo()`
- L197 `void MonitorK2SaveLoadCharacterSwap()`

Also 35 constants or variables at namespace scope.

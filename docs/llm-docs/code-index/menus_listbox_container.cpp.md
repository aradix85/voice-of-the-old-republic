# menus_listbox_container.cpp (239 lines)

Source: `patches/Accessibility/menus_listbox_container.cpp`

The container screen (loot, give items): its list box, Enter and Escape, the selection monitor and the give-mode key.

## Declarations (in source order)

- L37 `bool ContainerMatches(void* p)`
- L41 `void* ContainerFindLb(void* p)`
- L47 `void ContainerAnnounce(void* /*lb*/, const ListBoxNavResult& r)`: Inline announce only on a no-op clamp (boundary).
- L79 `bool ContainerOnEnter(void* panel)`
- L100 `bool ContainerOnEsc(void* panel)`
- L125 `void MonitorContainerSelection()`
- L215 `void PollContainerGiveModeKey()`

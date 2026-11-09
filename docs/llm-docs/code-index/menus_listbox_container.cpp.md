# menus_listbox_container.cpp (234 lines)

Source: `patches/Accessibility/menus_listbox_container.cpp`

The container screen (loot, give items): its list box, Enter and Escape, the selection monitor and the give-mode key.

## Declarations (in source order)

- L27 `bool ContainerMatches(void* p)`
- L31 `void* ContainerFindLb(void* p)`
- L37 `void ContainerAnnounce(void* /*lb*/, const ListBoxNavResult& r)`: Inline announce only on a no-op clamp (boundary).
- L69 `bool ContainerOnEnter(void* panel)`
- L90 `bool ContainerOnEsc(void* panel)`
- L115 `void MonitorContainerSelection()`
- L210 `void PollContainerGiveModeKey()`

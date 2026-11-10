# menus_monitors_internal.h (37 lines)

Source: `patches/Accessibility/menus_monitors_internal.h`

Internal to the menus_monitors module (menus_monitors*.cpp); no other file includes it. The public interface is menus_monitors.h. Here: what the files share.

## Declarations (in source order)

- L25 `void AnnounceNewSubScreens(void** panels, int count)`: menus_monitors_subscreens.cpp
- L28 `void MonitorPanelContents()`: menus_monitors_content.cpp
- L31 `void MonitorDialogReplies()`: menus_monitors_dialog.cpp
- L34 `void MonitorInventoryListPopulation()`: menus_monitors_inventory.cpp

Also 1 constants or variables at namespace scope.

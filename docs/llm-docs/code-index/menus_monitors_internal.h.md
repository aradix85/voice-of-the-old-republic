# menus_monitors_internal.h (65 lines)

Source: `patches/Accessibility/menus_monitors_internal.h`

Internal to the menus_monitors module (menus_monitors*.cpp); no other file includes it. The public interface is menus_monitors.h. Here: what the files share.

## Declarations (in source order)

- L40 `struct InGameSubScreenSpec`: Spec order = the icon strip's left-to-right visual order, which is what Tab / Shift+Tab cycle through.
- L52 `const InGameSubScreenSpec* FindSpec(PanelKind k)`: menus_monitors_subscreens.cpp
- L53 `void AnnounceNewSubScreens(void** panels, int count)`
- L56 `void MonitorPanelContents()`: menus_monitors_content.cpp
- L59 `void MonitorDialogReplies()`: menus_monitors_dialog.cpp
- L62 `void MonitorInventoryListPopulation()`: menus_monitors_inventory.cpp

Also 3 constants or variables at namespace scope.

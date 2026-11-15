# menus_listbox_workbench.cpp (329 lines)

Source: `patches/Accessibility/menus_listbox_workbench.cpp`

The workbench: the item list and the upgrade picker (empty rows, the KOTOR 2 'none' row, installing on Enter, the title).

## Declarations (in source order)

- L53 `bool WorkbenchItemsMatches(void* p)`
- L57 `void* WorkbenchItemsFindLb(void* p)`
- L65 `void WorkbenchItemsAnnounce(void* /*lb*/, const ListBoxNavResult& r)`: Speak the focused weapon row + position.
- L84 `bool WorkbenchItemsOnEnter(void* panel)`
- L90 `bool WorkbenchItemsOnEsc(void* panel)`
- L96 `bool WorkbenchUpgradeMatches(void* p)`
- L100 `bool WorkbenchUpgradeArmed()`
- L107 `void* WorkbenchUpgradeFindLb(void* p)`
- L123 `bool IsDashOrBlankRowText(const char* s)`: LB_ITEMS rows are CSWGuiInventoryItemEntry-style — their text comes from the item resref's localised name.
- L135 `void WorkbenchUpgradeAnnounce(void* /*lb*/, const ListBoxNavResult& r)`
- L179 `int WorkbenchUpgradeMinSel(void* panel)`: Dynamic minSel: hide the row-0 remove entry on power slots (minSel 1); show every row on the colour slot (minSel 0).
- L194 `bool WorkbenchUpgradeOnEnter(void* panel)`: Enter (armed): commit the currently-selected LB_ITEMS row via a single pending op that fires vtable[15] on the row (stage), then on BTN_ASSEMBLE (install).
- L260 `bool WorkbenchUpgradeOnEsc(void* panel)`: Esc (armed): disarm only.
- L279 `const char* WorkbenchUpgradeTitleOverride(void* panel)`: The generic title-walk would land on the first leftover placeholder label (id 19 "Item Property" — a design-time string the engine repopulates per slot at runtime, alongside id 20 "Slot Name" / id 21 "Lightsaber Slot ...

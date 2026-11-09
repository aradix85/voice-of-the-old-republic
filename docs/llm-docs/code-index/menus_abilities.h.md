# menus_abilities.h (59 lines)

Source: `patches/Accessibility/menus_abilities.h`

In-game "Fähigkeiten" screen (CSWGuiInGameAbilities, abilities.gui).

A view-only character screen shaped like the settings menu: three tab buttons (Powers / Skills / Talents) switch which list the single LB_ABILITY listbox shows, and selecting a row repaints a detail area (name + rank/bonus/total labels + the LB_DESC description box).

It is NOT the chargen/level-up button-grid shape — it rides the shared ListBoxPanelSpec machinery in menus_listbox.cpp (Up/Down browse LB_ABILITY) and the description-peek registry in peek_description.cpp (Shift+Up/Down pages LB_DESC). The detail labels and LB_DESC are repainted by calling the engine's CSWGuiInGameAbilities::OnAbilitySelectionChanged after we drive the listbox cursor (DriveListBoxSelection bypasses the engine's own onSelectionChanged), mirroring the chargen-skills "drive cursor → call engine handler → read the repainted detail" pattern.

## Declarations (in source order)

- L24 `bool IsAbilitiesPanel(void* panel)`: True iff `panel` is the in-game Fähigkeiten screen.
- L29 `void RefreshDetail(void* panel)`: Repaint the detail area (name/rank/bonus/total labels + LB_DESC) for the listbox's current selection.
- L55 `bool HandleInput(int n, void* thisPtr, void* activePanel, int param_1, int param_2, int& outRv)`: Dedicated input handler for the Fähigkeiten screen — a two-level submenu so the engine's mouse-only / crash-prone paths never see the keys: * Tab level (where you land): Up/Down move between the available tabs (Skills...

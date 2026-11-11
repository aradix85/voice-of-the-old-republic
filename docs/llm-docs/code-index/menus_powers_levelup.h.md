# menus_powers_levelup.h (25 lines)

Source: `patches/Accessibility/menus_powers_levelup.h`

The Force-power picker (CSWGuiPowersLevelUp), in character generation and in level-up: one engine class hosts both.

The panel is a chart of power families, not a flat list: its powers list box holds CSWGuiSkillFlow rows with up to three cells (the power, its improved and its master form), the same layout as the Feats screen. The keyboard model is the shared one in menus_skillflow_grid.h.

## Declarations (in source order)

- L13 `bool IsPowersLevelUpPanel(void* panel)`
- L19 `const char* GetTitleOverride(void* panel)`: The title to announce for this panel: its sub-title label, which names the powers screen, where the main title label of pwrlvlup.gui is the character generation header.
- L22 `bool HandleInput(void* panel, int code, int value, int& outRv)`: True when the event is consumed; `outRv` is then the input hook's result.

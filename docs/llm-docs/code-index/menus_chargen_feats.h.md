# menus_chargen_feats.h (19 lines)

Source: `patches/Accessibility/menus_chargen_feats.h`

The Feats screen of character generation and level-up (CSWGuiFeatsCharGen).

The screen is a chart of feat chains: a CSWGuiSkillFlowChart of rows with up to three cells (the feat, its improved and its master form), embedded as the single child of the feats list box. Each cell carries a feat id and a status. The keyboard model is the shared one in menus_skillflow_grid.h.

## Declarations (in source order)

- L13 `bool IsChargenFeatsPanel(void* panel)`: True iff `panel`'s vtable is CSWGuiFeatsCharGen's.
- L16 `bool HandleInput(void* panel, int code, int value, int& outRv)`: True when the event is consumed; `outRv` is then the input hook's result.

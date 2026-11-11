# menus_skillflow_grid.h (97 lines)

Source: `patches/Accessibility/menus_skillflow_grid.h`

The keyboard model of a skill-flow chart screen.

The feat chart of character generation and the Force-power chart of level-up are the same engine structure: CSWGuiSkillFlow rows of up to three cells (a chain: base, improved, master), a CSWGuiSkillFlowChart that holds the selection, a name label, a description list box and three buttons. The generic list-box handler does not fit them - the chart is not row-shaped - so they share this one: a 2D cursor over the filled cells, with the buttons as one-cell rows below the chart.

Up / Down     the row; keeps the column, or takes the nearest filled one Left / Right  the filled cells of the row Home / End    the first cell of the chart / the last button Enter         picks the cell (add or remove), or activates the button Esc           the back button

Moving onto a cell selects it in the engine: SetSelectedSkill on the chart for the highlight, and the panel's own OnEnter, which fills the name label and the description. Those are then spoken, with the cell's status.

## Declarations (in source order)

- L28 `struct Button`
- L35 `struct Screen`: What differs between the screens.
- L58 `class Grid`

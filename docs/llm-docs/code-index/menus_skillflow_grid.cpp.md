# menus_skillflow_grid.cpp (240 lines)

Source: `patches/Accessibility/menus_skillflow_grid.cpp`

(From menus_skillflow_grid.h.) The keyboard model of a skill-flow chart screen.

The feat chart of character generation and the Force-power chart of level-up are the same engine structure: CSWGuiSkillFlow rows of up to three cells (a chain: base, improved, master), a CSWGuiSkillFlowChart that holds the selection, a name label, a description list box and three buttons. The generic list-box handler does not fit them - the chart is not row-shaped - so they share this one: a 2D cursor over the filled cells, with the buttons as one-cell rows below the chart.

Up / Down     the row; keeps the column, or takes the nearest filled one Left / Right  the filled cells of the row Home / End    the first cell of the chart / the last button Enter         picks the cell (add or remove), or activates the button Esc           the back button

Moving onto a cell selects it in the engine: SetSelectedSkill on the chart for the highlight, and the panel's own OnEnter, which fills the name label and the description. Those are then spoken, with the cell's status.

## Declarations (in source order)

- L29 `void* CellOf(void* flow, int col)`
- L34 `unsigned short ReadCellId(void* flow, int col)`
- L42 `const char* ReadStatusWord(void* flow, int col, unsigned* outStatus)`: The chart's render status of a cell, the lowest byte of its status dword: 0 available, 1 already learned, 2 granted, 3 locked, 4 chosen.
- L60 `bool ReadDescription(void* listBox, char* out, size_t n)`: The rendered text of the description list box's first row, which the panel's OnEnter has just filled.
- L74 `unsigned short Grid::CurrentId() const`
- L79 `void Grid::ReadRows(void* rows, int count)`: Rows without a single cell are left out.
- L95 `void Grid::Bind(void* panel)`: The rows are read again on every event, so that a row pointer is never stale.
- L116 `void Grid::SelectInEngine(void* panel, unsigned short id)`
- L131 `void Grid::Announce(void* panel)`
- L173 `void Grid::Move(void* panel, const char* key, int row, int col)`: Announces also when the cursor could not move: the repeat says that this is the edge.
- L183 `void Grid::Activate(void* panel)`
- L206 `bool Grid::HandleInput(void* panel, int code, int value, int& outRv)`

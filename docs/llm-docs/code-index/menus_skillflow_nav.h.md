# menus_skillflow_nav.h (24 lines)

Source: `patches/Accessibility/menus_skillflow_nav.h`

Column search in one row of a skill-flow chart (the feat chart and the Force-power chart). A row is a chain of kColumns cells - base, improved, master - and kEmptyCell marks a cell the chain does not have.

## Declarations (in source order)

- L13 `int FirstFilledCol(const unsigned short* cells)`: The first filled column, or -1 for an empty row.
- L17 `int NearestFilledCol(const unsigned short* cells, int want)`: `want` when that cell is filled, else the filled column nearest to it (the lower one when two are as near), else -1.
- L21 `int StepCol(const unsigned short* cells, int from, int step)`: The next filled column from `from` in direction `step` (+1 or -1), or `from` itself at the end of the row.

Also 2 constants or variables at namespace scope.

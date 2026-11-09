# menus_skillflow_nav.h (28 lines)

Source: `patches/Accessibility/menus_skillflow_nav.h`

2D grid navigation primitives shared by the two CSWGuiSkillFlow-shaped menus (chargen feats grid, powers level-up grid). Each panel's row data lives in its own TU with a per-panel "empty cell" sentinel (featId vs powerId @ 0xffff); callers pass that cell predicate as a function pointer so the scan loops only have to be written once.

Column count is kSkillFlowColumnsPerRow from engine_offsets.h.

## Declarations (in source order)

- L16 `using IsFilledFn`: Cell predicate.
- L19 `int FirstFilledCol(int row, IsFilledFn isFilled)`: First filled column in chart row `row`, or -1 if none.
- L25 `int NearestFilledCol(int row, int want, IsFilledFn isFilled)`: Pick the column closest to `want` in chart row `row` among filled columns.

# menus_skillflow_nav.cpp (24 lines)

Source: `patches/Accessibility/menus_skillflow_nav.cpp`

(From menus_skillflow_nav.h.) 2D grid navigation primitives shared by the two CSWGuiSkillFlow-shaped menus (chargen feats grid, powers level-up grid). Each panel's row data lives in its own TU with a per-panel "empty cell" sentinel (featId vs powerId @ 0xffff); callers pass that cell predicate as a function pointer so the scan loops only have to be written once.

Column count is kSkillFlowColumnsPerRow from engine_offsets.h.

## Declarations (in source order)

- L7 `int FirstFilledCol(int row, IsFilledFn isFilled)`
- L14 `int NearestFilledCol(int row, int want, IsFilledFn isFilled)`

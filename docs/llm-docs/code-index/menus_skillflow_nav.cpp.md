# menus_skillflow_nav.cpp (37 lines)

Source: `patches/Accessibility/menus_skillflow_nav.cpp`

(From menus_skillflow_nav.h.) Column search in one row of a skill-flow chart (the feat chart and the Force-power chart). A row is a chain of kColumns cells - base, improved, master - and kEmptyCell marks a cell the chain does not have.

## Declarations (in source order)

- L7 `bool Filled(const unsigned short* cells, int col)`
- L13 `int FirstFilledCol(const unsigned short* cells)`
- L20 `int NearestFilledCol(const unsigned short* cells, int want)`
- L29 `int StepCol(const unsigned short* cells, int from, int step)`

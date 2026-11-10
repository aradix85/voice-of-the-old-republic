# menus_chargen_feats.cpp (562 lines)

Source: `patches/Accessibility/menus_chargen_feats.cpp`

chargen "Feats" main panel input handler. See menus_chargen_feats.h for the design rationale.

## Declarations (in source order)

- L114 `inline int TotalRowCount()`
- L115 `inline bool IsButtonRow(int r)`
- L119 `bool ColFilled(int r, int c)`: Cell predicate for the shared skillflow_nav helpers — feats grid empty sentinel is featId == 0xffff.
- L125 `int FirstFilledCol(int r)`
- L129 `int NearestFilledCol(int r, int want)`
- L135 `bool ReadChartBinding(void* panel, void*& outRows, int& outCount)`: Read the engine's current chart binding (rows array data pointer + size) for `panel`.
- L158 `void WalkChartRows(void* rows, int nRows)`: Walk the engine's chart rows array into s_chartRows[].
- L202 `void EnsureBound(void* panel)`: Re-walk the engine's chart on every hit so we never index stale row pointers (e.g.
- L244 `unsigned char ReadCellStatus(int r, int c)`: Lowest byte of the cell's 0x120 dword — the chart-render status enum (0 avail / 1 existing / 2 granted / 3 locked / 4 chosen).
- L260 `const char* StatusWord(unsigned char status)`
- L273 `bool ReadNameLabel(void* panel, char* out, size_t outN)`
- L281 `bool ReadDescription(void* panel, char* out, size_t outN)`: Read panel.description_listbox.controls[0] rendered text.
- L303 `void DriveEngineSelection(void* panel, unsigned short featId)`: SetSelectedSkill on the chart (visual highlight + chart's selected (col,row) state) + OnEnterFeat on the panel (refreshes name_label, description_listbox, BTN_SELECT label/colour for add/remove).
- L330 `void AnnounceFocused(void* panel)`
- L387 `void NavVertical(void* panel, bool down)`
- L411 `void NavHorizontal(void* panel, bool right)`
- L444 `void NavJumpEnd(void* panel, bool toEnd)`: Home / End across the whole 2D surface, not just the current row.
- L473 `bool IsChargenFeatsPanel(void* panel)`
- L484 `bool HandleInput(int n, void* thisPtr, void* panel, int param_1, int param_2, int& outRv)`

# menus_powers_levelup.cpp (511 lines)

Source: `patches/Accessibility/menus_powers_levelup.cpp`

Force-power picker input handler. See menus_powers_levelup.h for the design rationale.

## Declarations (in source order)

- L83 `inline int TotalRowCount()`
- L84 `inline bool IsButtonRow(int r)`
- L88 `bool ColFilled(int r, int c)`: Cell predicate for the shared skillflow_nav helpers — powers grid empty sentinel is powerId == 0xffff.
- L94 `int FirstFilledCol(int r)`
- L98 `int NearestFilledCol(int r, int want)`
- L108 `bool ReadChartBinding(void* panel, void*& outRows, int& outCount)`: Read the engine's current chart binding (powers_listbox.controls data pointer + size).
- L129 `void WalkChartRows(void* rows, int nRows)`: Walk the engine's chart rows array into s_chartRows[].
- L173 `void EnsureBound(void* panel)`: Re-walk the engine's chart on every hit so we never index stale row pointers when the same panel pointer is reused for the next character in the auto-level-up queue.
- L209 `unsigned char ReadCellStatus(int r, int c)`
- L225 `const char* StatusWord(unsigned char status)`
- L238 `bool ReadPowerName(void* panel, char* out, size_t outN)`
- L243 `bool ReadDescription(void* panel, char* out, size_t outN)`
- L266 `void DriveEngineSelection(void* panel, unsigned short powerId)`: SetSelectedSkill on the embedded chart + OnEnterPower on the panel.
- L293 `void AnnounceFocused(void* panel)`
- L344 `void NavVertical(void* panel, bool down)`
- L368 `void NavHorizontal(void* panel, bool right)`
- L395 `void NavJumpEnd(void* panel, bool toEnd)`: Home / End across the whole 2D surface — leftmost filled cell of the first power row, and the final button row (Abbrechen).
- L424 `bool IsPowersLevelUpPanel(void* panel)`
- L428 `const char* GetTitleOverride(void* panel)`
- L438 `bool HandleInput(int n, void* thisPtr, void* panel, int param_1, int param_2, int& outRv)`

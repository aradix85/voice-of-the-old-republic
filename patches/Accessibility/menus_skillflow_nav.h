// Column search in one row of a skill-flow chart (the feat chart and the
// Force-power chart). A row is a chain of kColumns cells - base, improved,
// master - and kEmptyCell marks a cell the chain does not have.

#pragma once

namespace acc::menus::skillflow_nav {

constexpr int kColumns = 3;
constexpr unsigned short kEmptyCell = 0xffff;

// The first filled column, or -1 for an empty row.
int FirstFilledCol(const unsigned short* cells);

// `want` when that cell is filled, else the filled column nearest to it (the
// lower one when two are as near), else -1.
int NearestFilledCol(const unsigned short* cells, int want);

// The next filled column from `from` in direction `step` (+1 or -1), or
// `from` itself at the end of the row.
int StepCol(const unsigned short* cells, int from, int step);

// How many filled cells the row has beyond `from` in direction `step`.
int FilledBeyond(const unsigned short* cells, int from, int step);

}  // namespace acc::menus::skillflow_nav

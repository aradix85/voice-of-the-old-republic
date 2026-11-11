#include "menus_skillflow_nav.h"

namespace acc::menus::skillflow_nav {

namespace {

bool Filled(const unsigned short* cells, int col) {
    return col >= 0 && col < kColumns && cells[col] != kEmptyCell;
}

}  // namespace

int FirstFilledCol(const unsigned short* cells) {
    for (int c = 0; c < kColumns; ++c) {
        if (Filled(cells, c)) return c;
    }
    return -1;
}

int NearestFilledCol(const unsigned short* cells, int want) {
    if (Filled(cells, want)) return want;
    for (int d = 1; d < kColumns; ++d) {
        if (Filled(cells, want - d)) return want - d;
        if (Filled(cells, want + d)) return want + d;
    }
    return FirstFilledCol(cells);
}

int StepCol(const unsigned short* cells, int from, int step) {
    for (int c = from + step; c >= 0 && c < kColumns; c += step) {
        if (Filled(cells, c)) return c;
    }
    return from;
}

}  // namespace acc::menus::skillflow_nav

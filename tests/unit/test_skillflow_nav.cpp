// menus_skillflow_nav: the column a cursor lands on in a feat or power chain.

#include "doctest.h"
#include "menus_skillflow_nav.h"

using namespace acc::menus::skillflow_nav;

namespace {

constexpr unsigned short E = kEmptyCell;

}  // namespace

TEST_CASE("the first filled column of a row") {
    const unsigned short full[]  = {10, 11, 12};
    const unsigned short late[]  = {E, E, 12};
    const unsigned short empty[] = {E, E, E};
    CHECK(FirstFilledCol(full) == 0);
    CHECK(FirstFilledCol(late) == 2);
    CHECK(FirstFilledCol(empty) == -1);
}

TEST_CASE("moving to another row keeps the column when that cell exists") {
    const unsigned short full[] = {10, 11, 12};
    CHECK(NearestFilledCol(full, 0) == 0);
    CHECK(NearestFilledCol(full, 2) == 2);
}

TEST_CASE("moving to another row takes the nearest filled cell otherwise") {
    const unsigned short baseOnly[] = {10, E, E};
    const unsigned short ends[]     = {10, E, 12};
    const unsigned short lastOnly[] = {E, E, 12};
    const unsigned short empty[]    = {E, E, E};
    CHECK(NearestFilledCol(baseOnly, 2) == 0);
    CHECK(NearestFilledCol(ends, 1) == 0);      // as near on both sides: the lower
    CHECK(NearestFilledCol(lastOnly, 0) == 2);
    CHECK(NearestFilledCol(empty, 1) == -1);
}

TEST_CASE("left and right step over gaps and stop at the ends of the row") {
    const unsigned short ends[] = {10, E, 12};
    CHECK(StepCol(ends, 0, +1) == 2);
    CHECK(StepCol(ends, 2, -1) == 0);
    CHECK(StepCol(ends, 2, +1) == 2);
    CHECK(StepCol(ends, 0, -1) == 0);

    const unsigned short baseOnly[] = {10, E, E};
    CHECK(StepCol(baseOnly, 0, +1) == 0);
}

TEST_CASE("the cells of a chain further along the row are counted") {
    const unsigned short full[]     = {10, 11, 12};
    const unsigned short ends[]     = {10, E, 12};
    const unsigned short baseOnly[] = {10, E, E};
    CHECK(FilledBeyond(full, 0, +1) == 2);
    CHECK(FilledBeyond(full, 1, +1) == 1);
    CHECK(FilledBeyond(full, 2, +1) == 0);
    CHECK(FilledBeyond(full, 2, -1) == 2);
    CHECK(FilledBeyond(ends, 0, +1) == 1);
    CHECK(FilledBeyond(baseOnly, 0, +1) == 0);
}

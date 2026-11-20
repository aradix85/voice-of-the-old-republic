// The play time in a save row's text (play_time).

#include "doctest.h"
#include "play_time.h"

using acc::menus::PlayMinutes;

TEST_CASE("The play time of a save row is read from its text") {
    CHECK(PlayMinutes("QUICKSAVE - 80h 24m") == 80 * 60 + 24);
    CHECK(PlayMinutes("Game 1 - 21h 23m") == 21 * 60 + 23);
    CHECK(PlayMinutes("AUTOSAVE - 0h 5m") == 5);
    CHECK(PlayMinutes("12h34m") == 12 * 60 + 34);
}

TEST_CASE("A number that is not a play time is passed over") {
    CHECK(PlayMinutes("Game 12 - 3h 7m") == 3 * 60 + 7);
    CHECK(PlayMinutes("000002 - Game 1 - 9h 59m") == 9 * 60 + 59);
}

TEST_CASE("A text without a play time has none") {
    CHECK(PlayMinutes("QUICKSAVE") == -1);
    CHECK(PlayMinutes("12h") == -1);
    CHECK(PlayMinutes("12h x") == -1);
    CHECK(PlayMinutes("") == -1);
    CHECK(PlayMinutes(nullptr) == -1);
}

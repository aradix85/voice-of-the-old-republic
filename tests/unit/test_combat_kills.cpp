// The kills of one fight, as the summary at its end names them (combat_kills).

#include "doctest.h"
#include "combat_kills.h"

using acc::combat::KillTally;
using acc::combat::KillTarget;

TEST_CASE("The target of a kill line stands between the marker and the experience") {
    CHECK(KillTarget("Mission killed Gamorrean Elite: 100 XP", "killed") == "Gamorrean Elite");
    CHECK(KillTarget("korasha killed Rakghoul", "killed") == "Rakghoul");
}

TEST_CASE("A line without the marker has no target") {
    CHECK(KillTarget("korasha uses Medpac", "killed").empty());
    CHECK(KillTarget(nullptr, "killed").empty());
    CHECK(KillTarget("Mission killed Gamorrean Elite: 100 XP", "").empty());
}

TEST_CASE("Kills are counted per name, in the order of the first kill") {
    KillTally tally;
    CHECK(tally.Empty());
    tally.Note("Gamorrean Elite");
    tally.Note("Gamorrean Chief");
    tally.Note("Gamorrean Elite");
    tally.Note("");
    CHECK(tally.List() == "2 Gamorrean Elite, Gamorrean Chief");
    tally.Clear();
    CHECK(tally.Empty());
    CHECK(tally.List().empty());
}

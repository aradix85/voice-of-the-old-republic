// dialog_speech_marker: the skill-check outcome marker of a dialog line.

#include <string>

#include "dialog_speech_marker.h"
#include "doctest.h"

using acc::dialog_speech::ExtractSkillCheckMarker;

TEST_CASE("an outcome marker is found and cut out") {
    char out[80];
    CHECK(ExtractSkillCheckMarker("[Success] Very well, I'll do it.", out, sizeof(out)));
    CHECK(std::string(out) == "Success");
    CHECK(ExtractSkillCheckMarker("  [COMPUTER SKILL: Success!] Access granted.", out,
                                  sizeof(out)));
    CHECK(std::string(out) == "COMPUTER SKILL: Success!");
    CHECK(ExtractSkillCheckMarker("[Failure: Insufficient Spikes \t]", out, sizeof(out)));
    CHECK(std::string(out) == "Failure: Insufficient Spikes");
    CHECK(ExtractSkillCheckMarker("[FAILURE]", out, sizeof(out)));
    CHECK(std::string(out) == "FAILURE");
}

TEST_CASE("other lines have no marker") {
    char out[80];
    CHECK_FALSE(ExtractSkillCheckMarker("[Persuade] Come on, help me out.", out, sizeof(out)));
    CHECK_FALSE(ExtractSkillCheckMarker("[laughs] Good one.", out, sizeof(out)));
    CHECK_FALSE(ExtractSkillCheckMarker("Success! You did it.", out, sizeof(out)));
    CHECK_FALSE(ExtractSkillCheckMarker("[Success without a close", out, sizeof(out)));
    CHECK_FALSE(ExtractSkillCheckMarker("[]", out, sizeof(out)));
    CHECK_FALSE(ExtractSkillCheckMarker(nullptr, out, sizeof(out)));
    CHECK_FALSE(ExtractSkillCheckMarker("[Success]", out, 1));
}

TEST_CASE("the marker is cut to the output buffer") {
    char out[5];
    CHECK(ExtractSkillCheckMarker("[Success]", out, sizeof(out)));
    CHECK(std::string(out) == "Succ");
}

// strfmt: heap-backed printf, the bounded append and the ASCII case folds.

#include <string>

#include "doctest.h"
#include "strfmt.h"

using acc::strfmt::AsciiLower;
using acc::strfmt::ContainsNoCase;
using acc::strfmt::FixedText;
using acc::strfmt::Format;
using acc::strfmt::ToLowerAscii;

TEST_CASE("Format fills a string of exactly the right length") {
    CHECK(Format("%s %d", "Door", 2) == "Door 2");
    CHECK(Format(nullptr).empty());
    CHECK(Format("").empty());
    const std::string longName(1000, 'a');
    CHECK(Format("To %s", longName.c_str()).size() == 1003);  // never cut off
}

TEST_CASE("FixedText appends until the buffer is full and stays terminated") {
    char buf[8];
    FixedText t{buf, sizeof(buf)};
    t.Append("%s", "ab");
    t.Append("%d", 12);
    CHECK(std::string(buf) == "ab12");
    CHECK(t.len == 4);
    t.Append("%s", "cdefgh");  // only three more fit
    CHECK(std::string(buf) == "ab12cde");
    CHECK(t.len == 7);
    t.Append("%s", "x");       // full: dropped
    CHECK(std::string(buf) == "ab12cde");
}

TEST_CASE("The case folds change ASCII letters only") {
    CHECK(ToLowerAscii("106PER") == "106per");
    CHECK(ToLowerAscii("Matrik's Apartment") == "matrik's apartment");
    CHECK(ToLowerAscii("\xC4\xD6") == "\xC4\xD6");  // non-ASCII bytes untouched
    CHECK(AsciiLower('Z') == 'z');
    CHECK(AsciiLower('[') == '[');
    CHECK(AsciiLower('@') == '@');
}

TEST_CASE("ContainsNoCase finds ASCII text in any case") {
    CHECK(ContainsNoCase("Steam Big Picture Mode", "big picture"));
    CHECK(ContainsNoCase("Primary Sound Driver (DirectSound)", "DIRECTSOUND"));
    CHECK(ContainsNoCase("EAX", "eax"));
    CHECK_FALSE(ContainsNoCase("Steam", "Big Picture"));
    CHECK_FALSE(ContainsNoCase("Big Pictur", "Big Picture"));  // needle runs past the end
    CHECK_FALSE(ContainsNoCase("@", "`"));  // only letters fold
    CHECK_FALSE(ContainsNoCase("anything", ""));
    CHECK_FALSE(ContainsNoCase(nullptr, "x"));
    CHECK_FALSE(ContainsNoCase("x", nullptr));
}

// strfmt: heap-backed printf and the ASCII case folds.

#include <string>

#include "doctest.h"
#include "strfmt.h"

using acc::strfmt::AsciiLower;
using acc::strfmt::ContainsNoCase;
using acc::strfmt::Format;
using acc::strfmt::ToLowerAscii;
using acc::strfmt::ToUpperAscii;

TEST_CASE("Format fills a string of exactly the right length") {
    CHECK(Format("%s %d", "Door", 2) == "Door 2");
    CHECK(Format(nullptr).empty());
    CHECK(Format("").empty());
    const std::string longName(1000, 'a');
    CHECK(Format("To %s", longName.c_str()).size() == 1003);  // never cut off
}

TEST_CASE("The case folds change ASCII letters only") {
    CHECK(ToLowerAscii("106PER") == "106per");
    CHECK(ToUpperAscii("po_pmha01") == "PO_PMHA01");
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

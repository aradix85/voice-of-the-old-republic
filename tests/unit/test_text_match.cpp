// text_match: rendered text against the TLK's wording of the same strref.

#include "doctest.h"
#include "text_match.h"

using acc::text::EqualsTrimmed;
using acc::text::MatchesWithTokens;

TEST_CASE("EqualsTrimmed ignores padding at both ends only") {
    CHECK(EqualsTrimmed(" Close", "Close"));       // KOTOR 2 pads its captions
    CHECK(EqualsTrimmed("Close\r\n", "\tClose "));
    CHECK_FALSE(EqualsTrimmed("Clo se", "Close"));  // inner spaces count
    CHECK_FALSE(EqualsTrimmed("close", "Close"));   // case counts
    CHECK_FALSE(EqualsTrimmed("Close", nullptr));
    CHECK(EqualsTrimmed("", "   "));
}

TEST_CASE("A token in the stored line matches whatever the engine put there") {
    const char* stored = "Welcome back, <FullName>. Your wounds are treated.";
    CHECK(MatchesWithTokens("Welcome back, Korasha. Your wounds are treated.", stored));
    CHECK(MatchesWithTokens(" Welcome back, Revan the Exile. Your wounds are treated. ", stored));
    CHECK_FALSE(MatchesWithTokens("Welcome back, Korasha. Your wounds are fine.", stored));
    CHECK_FALSE(MatchesWithTokens("Hello, Korasha. Your wounds are treated.", stored));
}

TEST_CASE("A trailing token swallows the rest, a final literal must end the line") {
    CHECK(MatchesWithTokens("Hello there, Carth and Bastila", "Hello there, <FullName>"));
    CHECK_FALSE(MatchesWithTokens("Hi Carth, more text", "Hi <FullName>,"));
    CHECK(MatchesWithTokens("Hi Carth,  ", "Hi <FullName>,"));
}

TEST_CASE("Without a token the token match is EqualsTrimmed") {
    CHECK(MatchesWithTokens("  Plain line ", "Plain line"));
    CHECK_FALSE(MatchesWithTokens("Plain line too", "Plain line"));
}

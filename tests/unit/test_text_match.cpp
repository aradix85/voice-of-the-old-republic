// text_match: rendered text against the TLK's wording of the same strref.

#include "doctest.h"
#include "text_match.h"

using acc::text::ContainsIgnoreCase;
using acc::text::EqualsTrimmed;
using acc::text::MatchesDialogLine;
using acc::text::MatchesWithTokens;
using acc::text::SameLastWord;

TEST_CASE("ContainsIgnoreCase finds a name inside a longer one") {
    CHECK(ContainsIgnoreCase("Dantooine - Matale Grounds", "Matale Grounds"));
    CHECK(ContainsIgnoreCase("Taris - Upper City South", "upper city"));
    CHECK(ContainsIgnoreCase("Ebon Hawk", "Ebon Hawk"));
    CHECK_FALSE(ContainsIgnoreCase("Dantooine - Jedi Enclave", "Exit Ramp"));
    CHECK_FALSE(ContainsIgnoreCase("Grove", "Dantooine - Grove"));  // needle longer
    CHECK_FALSE(ContainsIgnoreCase("Dantooine - Grove", ""));
    CHECK_FALSE(ContainsIgnoreCase(nullptr, "Grove"));
    CHECK_FALSE(ContainsIgnoreCase("Dantooine - Grove", nullptr));
}

TEST_CASE("SameLastWord matches two names for the same kind of thing") {
    CHECK(SameLastWord("Ancient Computer", "Rakata Computer", 4));
    CHECK(SameLastWord("Security terminal ", "Computer Terminal", 4));
    CHECK(SameLastWord("Computer", "Computer", 4));
    CHECK_FALSE(SameLastWord("Sith Base", "Sith Guard", 4));       // the first word shared
    CHECK_FALSE(SameLastWord("Computer Room", "Rakata Computer", 4));
    CHECK_FALSE(SameLastWord("North Bay", "Docking Bay", 4));      // the shared word too short
    CHECK_FALSE(SameLastWord("Computers", "Computer", 4));
    CHECK_FALSE(SameLastWord("", "", 4));
    CHECK_FALSE(SameLastWord(nullptr, "Computer", 4));
}

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

TEST_CASE("A dialogue line matches its TLK text with or without acting directions") {
    const char* stored = "{Equip weapon}Select the weapon slot, <FullName>.";
    CHECK(MatchesDialogLine("Select the weapon slot, Korasha.", stored));
    CHECK(MatchesDialogLine("{Equip weapon}Select the weapon slot, Korasha.", stored));
    CHECK(MatchesDialogLine("I see. Go on.", "I see. {Pause} Go on."));
    CHECK(MatchesDialogLine("I see.  Go on.", "I see. {Pause} Go on."));
    CHECK_FALSE(MatchesDialogLine("Select the armor slot, Korasha.", stored));
    CHECK_FALSE(MatchesDialogLine(nullptr, stored));
}

TEST_CASE("WithoutBraces drops a note in braces wherever it stands") {
    using acc::text::WithoutBraces;
    CHECK(WithoutBraces("Footlocker{Spikes}") == "Footlocker");
    CHECK(WithoutBraces("Blast Door {HK-50}") == "Blast Door");
    CHECK(WithoutBraces("Blast Door 1{HK-50} North") == "Blast Door 1 North");
    CHECK(WithoutBraces("{Dummy Medbay PC}test") == "test");
    CHECK(WithoutBraces("T3-M4 damages Footlocker{Spikes} for 3 damage") ==
          "T3-M4 damages Footlocker for 3 damage");
    CHECK(WithoutBraces("I see. {Pause} Go on.") == "I see. Go on.");
    CHECK(WithoutBraces("Plain name") == "Plain name");
    CHECK(WithoutBraces("Open { brace") == "Open { brace");
    CHECK(WithoutBraces("{Only a note}").empty());
}

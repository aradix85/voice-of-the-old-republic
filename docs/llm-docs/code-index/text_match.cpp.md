# text_match.cpp (137 lines)

Source: `patches/Accessibility/text_match.cpp`

Engine-free text comparisons (text_match.h).

## Declarations (in source order)

- L13 `std::string WithoutDirections(const char* s)`: A copy without acting directions in braces, each run of spaces left as one.
- L30 `const char* FindSegment(const char* hay, const char* needle, size_t n)`: strstr for a length-bounded needle.
- L40 `char Lower(char c)`
- L45 `const char* LastWord(const char* s, size_t& len)`: The last space-separated word of `s`, trailing spaces left out.
- L56 `bool EqualsTrimmed(const char* a, const char* b)`
- L68 `bool MatchesWithTokens(const char* rendered, const char* stored)`
- L107 `bool MatchesDialogLine(const char* rendered, const char* stored)`
- L114 `bool ContainsIgnoreCase(const char* haystack, const char* needle)`
- L124 `bool SameLastWord(const char* a, const char* b, size_t minLen)`

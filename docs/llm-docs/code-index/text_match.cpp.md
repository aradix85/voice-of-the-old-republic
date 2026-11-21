# text_match.cpp (112 lines)

Source: `patches/Accessibility/text_match.cpp`

Engine-free text comparisons (text_match.h).

## Declarations (in source order)

- L12 `const char* FindSegment(const char* hay, const char* needle, size_t n)`: strstr for a length-bounded needle.
- L22 `char Lower(char c)`
- L27 `const char* LastWord(const char* s, size_t& len)`: The last space-separated word of `s`, trailing spaces left out.
- L38 `bool EqualsTrimmed(const char* a, const char* b)`
- L50 `bool MatchesWithTokens(const char* rendered, const char* stored)`
- L89 `bool ContainsIgnoreCase(const char* haystack, const char* needle)`
- L99 `bool SameLastWord(const char* a, const char* b, size_t minLen)`

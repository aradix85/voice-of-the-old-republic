# text_match.cpp (76 lines)

Source: `patches/Accessibility/text_match.cpp`

Engine-free text comparisons (text_match.h).

## Declarations (in source order)

- L12 `const char* FindSegment(const char* hay, const char* needle, size_t n)`: strstr for a length-bounded needle.
- L24 `bool EqualsTrimmed(const char* a, const char* b)`
- L36 `bool MatchesWithTokens(const char* rendered, const char* stored)`

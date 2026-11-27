# text_match.cpp (145 lines)

Source: `patches/Accessibility/text_match.cpp`

Engine-free text comparisons (text_match.h).

## Declarations (in source order)

- L13 `const char* FindSegment(const char* hay, const char* needle, size_t n)`: strstr for a length-bounded needle.
- L23 `char Lower(char c)`
- L28 `const char* LastWord(const char* s, size_t& len)`: The last space-separated word of `s`, trailing spaces left out.
- L39 `bool EqualsTrimmed(const char* a, const char* b)`
- L51 `bool MatchesWithTokens(const char* rendered, const char* stored)`
- L90 `std::string WithoutBraces(const char* s)`
- L107 `bool MatchesDialogLine(const char* rendered, const char* stored)`
- L114 `bool ContainsIgnoreCase(const char* haystack, const char* needle)`
- L124 `bool SameLastWord(const char* a, const char* b, size_t minLen)`
- L136 `NoteNames NoteNamesThing(const char* note, const char* thing, size_t minLen)`

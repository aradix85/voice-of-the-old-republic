# text_match.h (37 lines)

Source: `patches/Accessibility/text_match.h`

Engine-free text comparisons, for matching engine-rendered text against our own TLK resolution of the same strref.

## Declarations (in source order)

- L19 `bool EqualsTrimmed(const char* a, const char* b)`: Case-sensitive compare that ignores leading/trailing whitespace on both sides.
- L26 `bool MatchesWithTokens(const char* rendered, const char* stored)`: EqualsTrimmed for lines the engine token-substitutes at render time ( and friends): the stored text's literal segments must appear left to right in the rendered line, a token matches any run of characters, and padding...
- L30 `bool ContainsIgnoreCase(const char* haystack, const char* needle)`: True when `needle` occurs in `haystack`, ASCII letter case ignored.
- L34 `bool SameLastWord(const char* a, const char* b, size_t minLen)`: True when both names end in the same word of at least `minLen` characters, ASCII letter case ignored: "Ancient Computer" and "Rakata Computer".

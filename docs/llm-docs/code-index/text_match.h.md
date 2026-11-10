# text_match.h (27 lines)

Source: `patches/Accessibility/text_match.h`

Engine-free text comparisons, for matching engine-rendered text against our own TLK resolution of the same strref.

## Declarations (in source order)

- L17 `bool EqualsTrimmed(const char* a, const char* b)`: Case-sensitive compare that ignores leading/trailing whitespace on both sides.
- L24 `bool MatchesWithTokens(const char* rendered, const char* stored)`: EqualsTrimmed for lines the engine token-substitutes at render time ( and friends): the stored text's literal segments must appear left to right in the rendered line, a token matches any run of characters, and padding...

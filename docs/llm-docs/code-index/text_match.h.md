# text_match.h (63 lines)

Source: `patches/Accessibility/text_match.h`

Engine-free text comparisons, for matching engine-rendered text against our own TLK resolution of the same strref.

## Declarations (in source order)

- L20 `bool EqualsTrimmed(const char* a, const char* b)`: Case-sensitive compare that ignores leading/trailing whitespace on both sides.
- L27 `bool MatchesWithTokens(const char* rendered, const char* stored)`: EqualsTrimmed for lines the engine token-substitutes at render time ( and friends): the stored text's literal segments must appear left to right in the rendered line, a token matches any run of characters, and padding...
- L34 `std::string WithoutBraces(const char* s)`: A copy without the notes in braces that KOTOR 2's writers and level designers left in the text: an acting direction in a dialogue line ("{Equip weapon}Select...") or a note in an object name ("Footlocker{Spikes}").
- L41 `bool MatchesDialogLine(const char* rendered, const char* stored)`: MatchesWithTokens for a dialogue line against its TLK text.
- L45 `bool ContainsIgnoreCase(const char* haystack, const char* needle)`: True when `needle` occurs in `haystack`, ASCII letter case ignored.
- L49 `bool SameLastWord(const char* a, const char* b, size_t minLen)`: True when both names end in the same word of at least `minLen` characters, ASCII letter case ignored: "Ancient Computer" and "Rakata Computer".
- L52 `enum class NoteNames`: How a map note's name stands to the name of something near it.
- L60 `NoteNames NoteNamesThing(const char* note, const char* thing, size_t minLen)`

# tutorial_hints_dialog.cpp (292 lines)

Source: `patches/Accessibility/tutorial_hints_dialog.cpp`

The dialogue-line side of the tutorial hints: the mouse-worded tutorial lines replaced by a keyboard hint popup, the voiceless subtitle lines forced spoken (K1), and the voiced lines whose subtitle is suppressed (K2). Every list is keyed by source strref and matched in the game's own wording through the TLK.

## Declarations (in source order)

- L122 `const DialogHint* ActiveDialogHints(int* count)`
- L200 `const char* FindSegment(const char* hay, const char* needle, size_t n)`: strstr for a length-bounded needle.
- L215 `bool MatchesWithTokens(const char* rendered, const char* stored)`: EqualsTrimmed for lines the engine token-substitutes at render time ( and friends, the treatinj medbay lines): the stored text's literal segments must appear left to right in the rendered line, a token matches any run...
- L256 `const char* HintForDialogLine(const char* renderedLine, uint32_t* outStrref)`
- L272 `bool IsForcedSpokenDialogLine(const char* renderedLine)`
- L282 `bool IsVoicedSuppressedLine(const char* renderedLine)`

# tutorial_hints_dialog.cpp (412 lines)

Source: `patches/Accessibility/tutorial_hints_dialog.cpp`

The dialogue-line side of the tutorial hints: the mouse-worded tutorial lines replaced by a keyboard hint popup, the voiceless subtitle lines forced spoken (K1), and the voiced lines whose subtitle is suppressed (K2).

## Declarations (in source order)

- L129 `inline const DialogHint* ActiveDialogHints(int* count)`: The running game's table.
- L153 `void BuildResolvedIfNeeded()`
- L201 `void BuildForcedTextIfNeeded()`
- L218 `const char* HintForDialogLine(const char* renderedLine, uint32_t* outStrref)`
- L238 `bool IsForcedSpokenDialogLine(const char* renderedLine)`
- L320 `void BuildVoicedTextIfNeeded()`
- L338 `const char* FindSegment(const char* hay, const char* needle, size_t n)`: memcmp-based strstr for a length-bounded needle segment.
- L354 `bool MatchesWithTokens(const char* rendered, const char* stored)`: EqualsTrimmed, extended for lines the engine token-substitutes at render time ( and friends, e.g.
- L395 `bool IsVoicedSuppressedLine(const char* renderedLine)`

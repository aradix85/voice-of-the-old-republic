# tutorial_hints.cpp (644 lines)

Source: `patches/Accessibility/tutorial_hints.cpp`

(From tutorial_hints.h.) Endar Spire / global tutorial keyboard-hint substitution.

The vanilla KOTOR tutorial is worded for mouse. Two surfaces carry it: * Silent tutorial.2da popups (TutorialBox) — game-wide. * Voice-acted Trask / "pop" dialogue on the Endar Spire.

We do NOT touch the on-screen text (a rest-sight player who reads the screen still gets the mouse wording). We substitute a keyboard-oriented line into the SPOKEN channel only: * Surface 1: menus_monitors reads the popup's tutorial row (+0x994) and, for a mapped row, speaks HintForTutorialRow() instead of the rendered mouse text. * Surface 2: dialog_speech matches the rendered line against the engine's TLK resolution of a known mouse-line strref and speaks HintForDialogLine().

Both paths are pure polls — no engine hooks, no dialog.tlk edits, no installer change. See docs and scratchpad tutorial inventory for the full mapping.

## Declarations (in source order)

- L137 `inline const DialogHint* ActiveDialogHints(int* count)`: The running game's table.
- L161 `void BuildResolvedIfNeeded()`
- L189 `int ReadTutorialRow(void* panel)`
- L199 `const char* HintForTutorialRow(int row)`
- L361 `inline const MouseMsg* ActiveMouseMsgs(int* count)`
- L378 `void BuildMouseTextIfNeeded()`
- L398 `const char* HintForMouseText(const char* text)`
- L433 `void BuildForcedTextIfNeeded()`
- L450 `const char* HintForDialogLine(const char* renderedLine, uint32_t* outStrref)`
- L470 `bool IsForcedSpokenDialogLine(const char* renderedLine)`
- L552 `void BuildVoicedTextIfNeeded()`
- L570 `const char* FindSegment(const char* hay, const char* needle, size_t n)`: memcmp-based strstr for a length-bounded needle segment.
- L586 `bool MatchesWithTokens(const char* rendered, const char* stored)`: EqualsTrimmed, extended for lines the engine token-substitutes at render time ( and friends, e.g.
- L627 `bool IsVoicedSuppressedLine(const char* renderedLine)`

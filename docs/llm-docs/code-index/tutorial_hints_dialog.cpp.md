# tutorial_hints_dialog.cpp (239 lines)

Source: `patches/Accessibility/tutorial_hints_dialog.cpp`

The dialogue-line side of the tutorial hints: the mouse-worded tutorial lines replaced by a keyboard hint popup, the voiceless subtitle lines forced spoken (K1), and the voiced lines whose subtitle is suppressed (K2). Every list is keyed by source strref and matched in the game's own wording through the TLK.

## Declarations (in source order)

- L124 `const DialogHint* ActiveDialogHints(int* count)`
- L203 `const char* HintForDialogLine(const char* renderedLine, uint32_t* outStrref)`
- L219 `bool IsForcedSpokenDialogLine(const char* renderedLine)`
- L229 `bool IsVoicedSuppressedLine(const char* renderedLine)`

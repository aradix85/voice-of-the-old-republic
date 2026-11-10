# tutorial_hints.cpp (258 lines)

Source: `patches/Accessibility/tutorial_hints.cpp`

(From tutorial_hints.h.) Endar Spire / global tutorial keyboard-hint substitution.

The vanilla KOTOR tutorial is worded for mouse. Two surfaces carry it: * Silent tutorial.2da popups (TutorialBox) — game-wide. * Voice-acted Trask / "pop" dialogue on the Endar Spire.

We do NOT touch the on-screen text (a rest-sight player who reads the screen still gets the mouse wording). We substitute a keyboard-oriented line into the SPOKEN channel only: * Surface 1: menus_monitors reads the popup's tutorial row (+0x994) and, for a mapped row, speaks HintForTutorialRow() instead of the rendered mouse text. * Surface 2: dialog_speech matches the rendered line against the engine's TLK resolution of a known mouse-line strref and speaks HintForDialogLine().

Both paths are pure polls — no engine hooks, no dialog.tlk edits, no installer change. See docs and scratchpad tutorial inventory for the full mapping.

## Declarations (in source order)

- L33 `int ReadTutorialRow(void* panel)`
- L43 `const char* HintForTutorialRow(int row)`
- L205 `inline const MouseMsg* ActiveMouseMsgs(int* count)`
- L222 `void BuildMouseTextIfNeeded()`
- L242 `const char* HintForMouseText(const char* text)`

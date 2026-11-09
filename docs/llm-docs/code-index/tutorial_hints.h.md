# tutorial_hints.h (70 lines)

Source: `patches/Accessibility/tutorial_hints.h`

Endar Spire / global tutorial keyboard-hint substitution.

The vanilla KOTOR tutorial is worded for mouse. Two surfaces carry it: * Silent tutorial.2da popups (TutorialBox) — game-wide. * Voice-acted Trask / "pop" dialogue on the Endar Spire.

We do NOT touch the on-screen text (a rest-sight player who reads the screen still gets the mouse wording). We substitute a keyboard-oriented line into the SPOKEN channel only: * Surface 1: menus_monitors reads the popup's tutorial row (+0x994) and, for a mapped row, speaks HintForTutorialRow() instead of the rendered mouse text. * Surface 2: dialog_speech matches the rendered line against the engine's TLK resolution of a known mouse-line strref and speaks HintForDialogLine().

Both paths are pure polls — no engine hooks, no dialog.tlk edits, no installer change. See docs and scratchpad tutorial inventory for the full mapping.

## Declarations (in source order)

- L28 `int ReadTutorialRow(void* tutorialBoxPanel)`: Read the tutorial.2da row index off a live TutorialBox panel (CSWGuiTutorialBox +0x994, uint8 — RE-confirmed).
- L32 `const char* HintForTutorialRow(int row)`: Keyboard-hint replacement for a silent tutorial popup, by row index.
- L40 `const char* HintForDialogLine(const char* renderedLine, uint32_t* outStrref = nullptr)`: Keyboard-hint replacement for a rendered dialogue line whose source strref is a known mouse-oriented tutorial line.
- L48 `const char* HintForMouseText(const char* text)`: If `text` is the raw (mouse-worded) message of a mapped tutorial popup — the engine TLK resolution of one of the mapped rows' message strrefs — return the row's keyboard hint; else nullptr.
- L56 `bool IsForcedSpokenDialogLine(const char* renderedLine)`: True iff `renderedLine` is one of the curated VO-less subtitle lines that must be spoken even though its speaker classifies as a voiced human (and would otherwise be silenced by the human-subtitle suppression).
- L67 `bool IsVoicedSuppressedLine(const char* renderedLine)`: The inverse curation: true iff `renderedLine` is a curated line that HAS real recorded voice-over but whose speaker the classifier cannot resolve (ownerless barks / cinematic-copy panels, speaker unresolvable), so the...

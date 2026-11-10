# tutorial_hints.cpp (235 lines)

Source: `patches/Accessibility/tutorial_hints.cpp`

The tutorial popup side of the tutorial hints: the keyboard hint spoken for a mouse-worded tutorial.2da popup, found by the popup's row or by its rendered message text.

## Declarations (in source order)

- L30 `int ReadTutorialRow(void* panel)`
- L40 `const char* HintForTutorialRow(int row)`
- L204 `const MouseMsg* ActiveMouseMsgs(int* count)`
- L220 `const char* HintForMouseText(const char* text)`

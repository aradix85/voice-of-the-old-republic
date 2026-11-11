# tutorial_hints.cpp (224 lines)

Source: `patches/Accessibility/tutorial_hints.cpp`

The tutorial popup side of the tutorial hints: the keyboard hint spoken for a mouse-worded tutorial.2da popup, found by the popup's row or by its rendered message text.

## Declarations (in source order)

- L24 `int ReadTutorialRow(void* panel)`
- L29 `const char* HintForTutorialRow(int row)`
- L193 `const MouseMsg* ActiveMouseMsgs(int* count)`
- L209 `const char* HintForMouseText(const char* text)`

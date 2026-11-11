# tutorial_hints.cpp (228 lines)

Source: `patches/Accessibility/tutorial_hints.cpp`

The tutorial popup side of the tutorial hints: the keyboard hint spoken for a mouse-worded tutorial.2da popup, found by the popup's row or by its rendered message text.

## Declarations (in source order)

- L23 `int ReadTutorialRow(void* panel)`
- L33 `const char* HintForTutorialRow(int row)`
- L197 `const MouseMsg* ActiveMouseMsgs(int* count)`
- L213 `const char* HintForMouseText(const char* text)`

# menus_chain_buttons.cpp (112 lines)

Source: `patches/Accessibility/menus_chain_buttons.cpp`

Finding a panel's buttons by what they say (Close, Cancel) and the arrow buttons next to a control.

## Declarations (in source order)

- L26 `bool LabelOpensWithWord(const char* text, const char* word)`: Does `text` OPEN WITH the whole word `word`?
- L37 `void* FindButtonByWords(void* panel, const char* const* words, int wordCount)`: Shared scan behind both finders below: first chain-navigable control whose caption opens with one of `words`.
- L57 `void* FindAdjacentArrow(void* panel, void* focused, bool toRight)`
- L99 `void* FindCloseButton(void* panel)`
- L105 `void* FindCancelButton(void* panel)`

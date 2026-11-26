# menus_chain_buttons.cpp (116 lines)

Source: `patches/Accessibility/menus_chain_buttons.cpp`

Finding a panel's buttons by what they say (Close, Cancel) and the arrow buttons next to a control.

## Declarations (in source order)

- L27 `bool LabelOpensWithWord(const char* text, const char* word)`: Does `text` OPEN WITH the whole word `word`?
- L39 `void* FindButtonByWords(void* panel, const char* const* words, int wordCount)`: Shared scan behind both finders below: first chain-navigable control whose caption opens with one of `words`.
- L61 `void* FindAdjacentArrow(void* panel, void* focused, bool toRight)`: The nearest text-less control (an arrow button) on the same row as `focused`, to its right or left.
- L103 `void* FindCloseButton(void* panel)`
- L109 `void* FindCancelButton(void* panel)`

# unified_action_menu_speech.cpp (131 lines)

Source: `patches/Accessibility/unified_action_menu_speech.cpp`

What the unified action menu says: a category with its selected entry, an entry, an item's quantity, and an entry's description.

## Declarations (in source order)

- L21 `void AppendItemQuantity(void* tam, void* mi, const Cat& c, int idx, char* label, size_t n)`: Append ", N in stack" / ", N charges" to an item entry's label, matching the inventory / store suffixes.
- L46 `void FormatCategory(void* tam, void* mi, const Cat& c, int count, int idx, char* out, size_t n)`: Build the full category announce ("Name: label, N options" / "label, N options" for unnamed rows) into `out`.
- L76 `void SpeakCategory(void* tam, void* mi, const char* prefix)`: Speak the current category in full.
- L101 `void SpeakEntry(void* tam, void* mi)`: Speak only the current entry label — used after Up/Down/Home/End (the category didn't change, so its name would be redundant noise).
- L113 `void DescribeSelectedEntry(void* tam, void* mi, const Cat& cur, int sel)`: Shift + any arrow — describe the selected entry, no movement.

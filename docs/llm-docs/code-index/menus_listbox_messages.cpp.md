# menus_listbox_messages.cpp (196 lines)

Source: `patches/Accessibility/menus_listbox_messages.cpp`

The in-game messages screen: its views (left and right switch them), the list and its title.

## Declarations (in source order)

- L42 `void* MsgViewListBox(void* panel, MsgView v)`
- L54 `void* MsgViewButton(void* panel, MsgView v)`: The engine view button that shows `v`.
- L67 `bool ReadMsgViewName(void* panel, MsgView v, char* out, size_t outN)`: The name the screen itself shows for the stop: the view button's caption, or for the two effects lists their headings ("Gute Effekte" / ...).
- L77 `MsgView CurrentMsgView(void* panel)`
- L95 `void FormatMsgView(void* panel, MsgView v, char* out, size_t outN)`: " , N Einträge" / " , leer".
- L108 `bool InGameMessagesOnLeftRight(void* panel, int dir)`
- L141 `bool InGameMessagesMatches(void* p)`
- L145 `void* InGameMessagesFindLb(void* p)`
- L157 `void InGameMessagesAnnounce(void* /*lb*/, const ListBoxNavResult& r)`: NB: no `rowCount <= 0` guard here, unlike the other two users of SpeakRowAndPosition.
- L161 `const char* InGameMessagesTitleOverride(void* panel)`

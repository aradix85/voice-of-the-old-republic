# menus_listbox_messages.cpp (205 lines)

Source: `patches/Accessibility/menus_listbox_messages.cpp`

The in-game messages screen: its views (left and right switch them), the list and its title.

## Declarations (in source order)

- L51 `void* MsgViewListBox(void* panel, MsgView v)`
- L63 `void* MsgViewButton(void* panel, MsgView v)`: The engine view button that shows `v`.
- L76 `bool ReadMsgViewName(void* panel, MsgView v, char* out, size_t outN)`: The name the screen itself shows for the stop: the view button's caption, or for the two effects lists their headings ("Good Effects" / ...).
- L86 `MsgView CurrentMsgView(void* panel)`
- L104 `void FormatMsgView(void* panel, MsgView v, char* out, size_t outN)`: " , N entries" / " , empty".
- L117 `bool InGameMessagesOnLeftRight(void* panel, int dir)`
- L150 `bool InGameMessagesMatches(void* p)`
- L154 `void* InGameMessagesFindLb(void* p)`
- L166 `void InGameMessagesAnnounce(void* /*lb*/, const ListBoxNavResult& r)`: NB: no `rowCount <= 0` guard here, unlike the other two users of SpeakRowAndPosition.
- L170 `const char* InGameMessagesTitleOverride(void* panel)`

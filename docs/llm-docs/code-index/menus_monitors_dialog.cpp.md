# menus_monitors_dialog.cpp (254 lines)

Source: `patches/Accessibility/menus_monitors_dialog.cpp`

The dialogue reply monitor: speaking the selected reply with its position, on navigation and when the engine swaps in the next node's replies.

## Declarations (in source order)

- L40 `unsigned ComputeReplySetSignature(int replyCount, int rowCount)`: FNV-1a over every active reply's text plus the count.
- L55 `bool ParkDialogCursorOffReplies(void* replyLb)`
- L65 `bool IsDialogPanelKind(PanelKind k)`
- L79 `void MonitorDialogReplies()`

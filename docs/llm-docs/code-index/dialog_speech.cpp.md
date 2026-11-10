# dialog_speech.cpp (449 lines)

Source: `patches/Accessibility/dialog_speech.cpp`

transient (engine LYT loader use-after-free)

## Declarations (in source order)

- L32 `DialogPanelMatch FindActiveDialogPanel()`
- L70 `void* FindBarkBubblePanel()`: Find a BarkBubble panel (independent lifecycle from main dialog).
- L75 `int ReadListBoxRowCount(void* panel, size_t lbOffset)`: Read row count of a CSWGuiListBox at panel + offset.
- L93 `bool ReadFirstVisibleText(void* panel, char* outBuf, size_t bufSize)`: Walk a panel's controls[] for the first child whose text resolves non-empty.
- L129 `void TickComputerRows(void* panel)`: For the Computer dialog variant, also speak the message_listbox (terminal output) when its row count grows.
- L179 `void TickDialogPanel()`: ---- NPC line + replies count for the main dialog panel.
- L335 `void TickCameraFeed()`: ---- Security-camera feed — its own panel, its own lifecycle.
- L363 `void TickBark()`: ---- BarkBubble — independent lifecycle.
- L437 `void Tick()`

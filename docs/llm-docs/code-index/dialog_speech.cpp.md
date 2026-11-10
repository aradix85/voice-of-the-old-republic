# dialog_speech.cpp (362 lines)

Source: `patches/Accessibility/dialog_speech.cpp`

Dialog narration, polled each tick (see dialog_speech.h).

## Declarations (in source order)

- L34 `DialogPanelMatch FindActiveDialogPanel()`: The first dialog-kind panel in the manager's panels[].
- L64 `int ReadListBoxRowCount(void* panel, size_t lbOffset)`
- L80 `bool ReadFirstVisibleText(void* panel, char* outBuf, size_t bufSize)`: The text of a panel's first child that has any.
- L109 `bool RecordTutorialLine(const char* line)`: Records a tutorial line's keyboard hint for the popup fired at the next reply break or bark close.
- L121 `bool SilenceLine(bool suppressible, bool tutorialManaged, bool voicedLine, bool forcedSpoken)`: A line stays silent when its speaker's voice-over already says it, when the tutorial popup carries it, or when it is a curated voiced line whose speaker can't be resolved (the KOTOR 2 prologue narrator); unless it is ...
- L134 `void TickComputerRows(void* panel)`: A computer terminal's output rows, spoken as they are appended.
- L182 `void TickDialogPanel()`: The main dialog panel: the NPC line, a terminal's output, and R to repeat the line.
- L278 `void TickCameraFeed()`: A console's "view camera" swaps the terminal for CSWGuiDialogComputerCamera: a live picture whose one control, LBL_RETURN, holds the engine's own "press Enter to cancel the live feed and return".
- L303 `void TickBark()`: Bark bubbles, with their own lifecycle.
- L352 `void Tick()`

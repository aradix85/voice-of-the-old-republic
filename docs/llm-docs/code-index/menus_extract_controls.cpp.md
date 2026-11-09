# menus_extract_controls.cpp (626 lines)

Source: `patches/Accessibility/menus_extract_controls.cpp`

Text of the plain GUI controls: tooltip, button, toggle button, label, slider (with its sibling label), edit box, single-row list box, item entry row, and a label beside a cycle control's arrows.

## Declarations (in source order)

- L34 `bool IsCycleFlankerArrow(void* panel, void* control)`: True for image-only buttons that flank a cycle value-display button at the same y-row (e.g.
- L107 `const char* FindSiblingLabel(void* panel, void* control, char* outBuf, size_t bufSize)`: Find the closest label-like sibling of `control` on the panel.
- L224 `bool IsSoundOptionsMovieSlider(void* panel, void* control)`: Identify the movie-volume slider on optionssound.gui.
- L275 `const char* TryTooltip(void* control, char* outBuf, size_t bufSize)`: 1.
- L299 `const char* TryButton(void* control, char* outBuf, size_t bufSize)`: 2.
- L315 `const char* TryButtonToggle(void* control, char* outBuf, size_t bufSize)`: 3.
- L330 `const char* TryLabel(void* control, char* outBuf, size_t bufSize)`: 4.
- L345 `const char* TryLabelHilight(void* control, char* outBuf, size_t bufSize)`: 5.
- L383 `const char* TrySlider(void* control, void* owner, char* outBuf, size_t bufSize)`: 6.
- L425 `const char* TryEditbox(void* control, char* outBuf, size_t bufSize)`: 6b.
- L470 `const char* TrySingleRowListBox(void* control, char* outBuf, size_t bufSize)`: 7.
- L544 `const char* TryItemEntryRow(void* control, char* outBuf, size_t bufSize)`: 9.
- L598 `const char* TrySiblingLabel(void* control, void* owner, char* outBuf, size_t bufSize)`: 10.

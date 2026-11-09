# menus_extract_controls.cpp (623 lines)

Source: `patches/Accessibility/menus_extract_controls.cpp`

Text of the plain GUI controls: tooltip, button, toggle button, label, slider (with its sibling label), edit box, single-row list box, item entry row, and a label beside a cycle control's arrows.

## Declarations (in source order)

- L31 `bool IsCycleFlankerArrow(void* panel, void* control)`: True for image-only buttons that flank a cycle value-display button at the same y-row (e.g.
- L104 `const char* FindSiblingLabel(void* panel, void* control, char* outBuf, size_t bufSize)`: Find the closest label-like sibling of `control` on the panel.
- L221 `bool IsSoundOptionsMovieSlider(void* panel, void* control)`: Identify the movie-volume slider on optionssound.gui.
- L272 `const char* TryTooltip(void* control, char* outBuf, size_t bufSize)`: 1.
- L296 `const char* TryButton(void* control, char* outBuf, size_t bufSize)`: 2.
- L312 `const char* TryButtonToggle(void* control, char* outBuf, size_t bufSize)`: 3.
- L327 `const char* TryLabel(void* control, char* outBuf, size_t bufSize)`: 4.
- L342 `const char* TryLabelHilight(void* control, char* outBuf, size_t bufSize)`: 5.
- L380 `const char* TrySlider(void* control, void* owner, char* outBuf, size_t bufSize)`: 6.
- L422 `const char* TryEditbox(void* control, char* outBuf, size_t bufSize)`: 6b.
- L467 `const char* TrySingleRowListBox(void* control, char* outBuf, size_t bufSize)`: 7.
- L541 `const char* TryItemEntryRow(void* control, char* outBuf, size_t bufSize)`: 9.
- L595 `const char* TrySiblingLabel(void* control, void* owner, char* outBuf, size_t bufSize)`: 10.

# menus_extract_controls.cpp (288 lines)

Source: `patches/Accessibility/menus_extract_controls.cpp`

Text of the plain GUI controls: button, toggle button, label, slider (with its sibling label), edit box, single-row list box, item entry row, and a label beside a cycle control's arrows.

## Declarations (in source order)

- L26 `Controls PanelControls(void* panel)`
- L40 `bool IsCycleFlankerArrow(void* panel, void* control)`: True for an image-only button flanking a cycle's value button on the same row (the arrows around "Normal" in the Difficulty cycle): no text of its own, and a button with text within FindAdjacentArrow's reach.
- L77 `bool FindSiblingLabel(void* panel, void* control, char* outBuf, size_t bufSize)`: The text of the closest captioned label beside `control`: on the same row (left preferred, right as a fallback for trailing captions) or above or below it with a similar x (the Sound panel's label over each slider, th...
- L122 `bool IsSoundOptionsMovieSlider(void* panel, void* control)`: The movie-volume slider on KOTOR 1's optionssound.gui, whose stock label repeats the music slider's ("music volume").
- L137 `const char* TryAs(void* control, int vtableIndex, bool (*read)(void*, char*, size_t), const char* tag, char* outBuf, size_t bufSize)`: `tag` when `read` finds text on the control's downcast at `vtableIndex`.
- L146 `bool ReadRowText(void* row, char* outBuf, size_t bufSize)`: A list-box row's own text: the inline string or the strref of the row as a button, label or highlight label.
- L171 `const char* TryButton(void* control, char* outBuf, size_t bufSize)`: CSWGuiButton, which also covers CharButton, ActivatedButton and ButtonToggle: they embed Button at offset 0 and AsButton returns them.
- L176 `const char* TryButtonToggle(void* control, char* outBuf, size_t bufSize)`: CSWGuiButtonToggle, in case AsButton misses one (same text offsets).
- L181 `const char* TryLabel(void* control, char* outBuf, size_t bufSize)`
- L186 `const char* TryLabelHilight(void* control, char* outBuf, size_t bufSize)`: CSWGuiLabelHilight embeds Label at offset 0: same text offsets.
- L197 `const char* TrySlider(void* control, void* owner, char* outBuf, size_t bufSize)`: CSWGuiSlider, found by vtable (there is no AsSlider).
- L219 `const char* TryEditbox(void* control, char* outBuf, size_t bufSize)`: CSWGuiEditbox (chargen Name and the save-name popup), found by vtable: "{role}.
- L233 `const char* TrySingleRowListBox(void* control, char* outBuf, size_t bufSize)`: A list box with exactly one row: many modals (OK/Cancel boxes, the quit confirmation, tutorial popups) put their message there.
- L254 `const char* TryItemEntryRow(void* control, char* outBuf, size_t bufSize)`: An inventory, container or store row named from its ITEM when the row's own string is empty.
- L272 `const char* TrySiblingLabel(void* control, void* owner, char* outBuf, size_t bufSize)`: Last resort for a chain-navigable control without text, such as the in-game menu's image-only icons: speak the nearest label as its name.

# menus_extract_speculative.cpp (80 lines)

Source: `patches/Accessibility/menus_extract_speculative.cpp`

The speculative text read for controls whose class overrides AsLabel/AsButton, so the downcast returns null although the control is label- or button-shaped at the field level: the in-game menu icons (8 labels and 8 image-only buttons), chargen class icons and decorations. Only allow-listed vtables are tried.

## Declarations (in source order)

- L34 `bool ReadAt(void* control, size_t textOff, size_t strrefOff, size_t textObjOff, char* text, size_t size)`: Inline CExoString, then strref, then the text_object's own text or strref (labelhilights route their rendered text through it).
- L45 `const char* TrySpeculativeVtableRead(void* control, char* outBuf, size_t bufSize)`

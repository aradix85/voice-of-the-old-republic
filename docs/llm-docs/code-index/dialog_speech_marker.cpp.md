# dialog_speech_marker.cpp (70 lines)

Source: `patches/Accessibility/dialog_speech_marker.cpp`

The skill-check outcome marker at the start of a dialog line.

## Declarations (in source order)

- L25 `char FoldByte(unsigned char c)`: Lowercase ASCII for matching; other bytes pass through.
- L34 `bool ExtractSkillCheckMarker(const char* line, char* out, size_t outSize)`: If `line` opens with a bracketed skill-check outcome marker, copy the marker's inner text (verbatim, brackets removed, trailing whitespace trimmed) into `out` and return true.

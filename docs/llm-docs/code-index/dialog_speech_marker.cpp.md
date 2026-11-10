# dialog_speech_marker.cpp (53 lines)

Source: `patches/Accessibility/dialog_speech_marker.cpp`

The skill-check outcome marker at the start of a dialog line (see dialog_speech_marker.h).

## Declarations (in source order)

- L13 `char FoldByte(unsigned char c)`: Lowercase ASCII for matching; other bytes pass through.
- L19 `bool ExtractSkillCheckMarker(const char* line, char* out, size_t outSize)`

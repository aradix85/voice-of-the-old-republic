# dialog_speech_marker.h (23 lines)

Source: `patches/Accessibility/dialog_speech_marker.h`

The skill-check outcome marker at the start of a dialog line. Engine-free.

Persuade, Computer Use, Repair and other check results are ordinary NPC lines whose subtitle begins with a bracketed marker: [Success], [Failure], uppercase variants, a trailing "!", and compound forms like [COMPUTER SKILL: Success!] or [Failure: Insufficient Spikes]. When the line is VO-suppressed, the marker alone is spoken so the player still learns the outcome.

## Declarations (in source order)

- L20 `bool ExtractSkillCheckMarker(const char* line, char* out, size_t outSize)`: If `line` opens with a bracketed skill-check outcome marker, copy its inner text (brackets removed, trailing whitespace trimmed, cut to the buffer) into `out` and return true.

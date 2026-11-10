// The skill-check outcome marker at the start of a dialog line. Engine-free.
//
// Persuade, Computer Use, Repair and other check results are ordinary NPC
// lines whose subtitle begins with a bracketed marker: [Success], [Failure],
// uppercase variants, a trailing "!", and compound forms like
// [COMPUTER SKILL: Success!] or [Failure: Insufficient Spikes]. When the line
// is VO-suppressed, the marker alone is spoken so the player still learns the
// outcome.

#pragma once

#include <cstddef>

namespace acc::dialog_speech {

// If `line` opens with a bracketed skill-check outcome marker, copy its inner
// text (brackets removed, trailing whitespace trimmed, cut to the buffer)
// into `out` and return true. A bracket token without an outcome word
// ([Persuade], [laughs]) answers false, so a suppressed line never leaks.
bool ExtractSkillCheckMarker(const char* line, char* out, size_t outSize);

}  // namespace acc::dialog_speech

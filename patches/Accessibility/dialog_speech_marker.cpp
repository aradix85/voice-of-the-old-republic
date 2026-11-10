// The skill-check outcome marker at the start of a dialog line (see
// dialog_speech_marker.h).

#include "dialog_speech_marker.h"

#include <cstring>

namespace acc::dialog_speech {

namespace {

// Lowercase ASCII for matching; other bytes pass through.
char FoldByte(unsigned char c) {
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + 32) : static_cast<char>(c);
}

}  // namespace

bool ExtractSkillCheckMarker(const char* line, char* out, size_t outSize) {
    if (!line || !out || outSize < 2) return false;
    const unsigned char* p = reinterpret_cast<const unsigned char*>(line);
    while (*p == ' ' || *p == '\t') ++p;
    if (*p != '[') return false;
    ++p;  // past '['

    char inner[80];
    char folded[80];
    size_t n = 0;
    while (p[n] && p[n] != ']' && n < sizeof(inner) - 1) {
        inner[n]  = static_cast<char>(p[n]);
        folded[n] = FoldByte(p[n]);
        ++n;
    }
    if (p[n] != ']' || n == 0) return false;  // no close bracket, or empty/over-long
    inner[n]  = '\0';
    folded[n] = '\0';

    // Either outcome counts: the marker is spoken whichever way it went.
    if (!std::strstr(folded, "succe") && !std::strstr(folded, "failure")) {
        return false;
    }

    size_t len = n;
    while (len && (inner[len - 1] == ' ' || inner[len - 1] == '\t')) --len;
    if (len == 0) return false;
    if (len > outSize - 1) len = outSize - 1;
    std::memcpy(out, inner, len);
    out[len] = '\0';
    return true;
}

}  // namespace acc::dialog_speech

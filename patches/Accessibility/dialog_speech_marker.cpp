// The skill-check outcome marker at the start of a dialog line.

#include "dialog_speech_internal.h"

namespace acc::dialog_speech {

namespace {

// ---- Skill-check outcome marker ------------------------------------------
//
// Persuade / Computer-Use / Repair etc. results are authored as ordinary NPC
// entry nodes whose subtitle text *begins* with a bracketed marker:
// [Success] / [Failure], uppercase variants, a trailing "!", and compound
// forms like [COMPUTER SKILL: Success!] / [Failure: Insufficient Spikes].
//
// When such a line is VO-suppressed the whole subtitle — marker included —
// goes silent, so the player never learns whether the check passed. We detect
// the leading bracket token and, if it denotes a check outcome, speak just
// that token (never the suppressed speech that follows the "]"). A token
// without an outcome word (a skill-only tag like [Persuade], a stage
// direction like [laughs]) stays silent, so a suppressed line is never
// partially leaked.

// Lowercase ASCII for matching; other bytes pass through.
char FoldByte(unsigned char c) {
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + 32) : static_cast<char>(c);
}

}  // namespace

// If `line` opens with a bracketed skill-check outcome marker, copy the
// marker's inner text (verbatim, brackets removed, trailing whitespace
// trimmed) into `out` and return true. Otherwise return false.
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

    // Any match means "speak this marker"; the direction does not matter.
    static const char* kOutcomeRoots[] = {"succe", "failure"};
    bool isMarker = false;
    for (const char* r : kOutcomeRoots) {
        if (std::strstr(folded, r)) { isMarker = true; break; }
    }
    if (!isMarker) return false;

    char* s = inner;
    size_t len = std::strlen(s);
    while (len && (s[len - 1] == ' ' || s[len - 1] == '\t')) s[--len] = '\0';
    std::strncpy(out, s, outSize - 1);
    out[outSize - 1] = '\0';
    return out[0] != '\0';
}

}  // namespace acc::dialog_speech

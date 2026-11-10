// Engine-free text comparisons (text_match.h).

#include "text_match.h"

#include <cstring>

namespace acc::text {

namespace {

// strstr for a length-bounded needle.
const char* FindSegment(const char* hay, const char* needle, size_t n) {
    if (n == 0) return hay;
    const size_t hayLen = std::strlen(hay);
    if (hayLen < n) return nullptr;
    for (size_t i = 0; i + n <= hayLen; ++i) {
        if (std::memcmp(hay + i, needle, n) == 0) return hay + i;
    }
    return nullptr;
}

}  // namespace

bool EqualsTrimmed(const char* a, const char* b) {
    if (!a || !b) return false;
    while (*a == ' ' || *a == '\t' || *a == '\n' || *a == '\r') ++a;
    while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') ++b;
    size_t la = std::strlen(a), lb = std::strlen(b);
    while (la > 0 && (a[la - 1] == ' ' || a[la - 1] == '\t' ||
                      a[la - 1] == '\n' || a[la - 1] == '\r')) --la;
    while (lb > 0 && (b[lb - 1] == ' ' || b[lb - 1] == '\t' ||
                      b[lb - 1] == '\n' || b[lb - 1] == '\r')) --lb;
    return la == lb && std::memcmp(a, b, la) == 0;
}

bool MatchesWithTokens(const char* rendered, const char* stored) {
    if (std::strchr(stored, '<') == nullptr) return EqualsTrimmed(rendered, stored);
    while (*rendered == ' ') ++rendered;
    while (*stored == ' ') ++stored;
    const char* r = rendered;
    const char* s = stored;
    bool first = true;
    bool endsWithToken = false;
    while (*s) {
        if (*s == '<') {
            const char* close = std::strchr(s, '>');
            if (!close) { endsWithToken = true; break; }  // malformed tail
            s = close + 1;
            endsWithToken = (*s == '\0');
            first = false;
            continue;
        }
        const char* tok = std::strchr(s, '<');
        const size_t segLen = tok ? static_cast<size_t>(tok - s) : std::strlen(s);
        size_t cmpLen = segLen;
        if (!tok) {  // the final literal: its trailing padding does not count
            while (cmpLen > 0 && s[cmpLen - 1] == ' ') --cmpLen;
        }
        if (first) {
            if (std::strncmp(r, s, cmpLen) != 0) return false;
            r += cmpLen;
        } else {
            const char* hit = FindSegment(r, s, cmpLen);
            if (!hit) return false;
            r = hit + cmpLen;
        }
        s += segLen;
        first = false;
    }
    if (endsWithToken) return true;  // a trailing token swallows the rest
    while (*r == ' ') ++r;
    return *r == '\0';
}

}  // namespace acc::text

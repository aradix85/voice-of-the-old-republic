// Engine-free text comparisons (text_match.h).

#include "text_match.h"

#include <cstring>
#include <string>

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

char Lower(char c) {
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

// The last space-separated word of `s`, trailing spaces left out.
const char* LastWord(const char* s, size_t& len) {
    size_t end = std::strlen(s);
    while (end > 0 && s[end - 1] == ' ') --end;
    size_t begin = end;
    while (begin > 0 && s[begin - 1] != ' ') --begin;
    len = end - begin;
    return s + begin;
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

std::string WithoutBraces(const char* s) {
    std::string out;
    for (; *s; ++s) {
        if (*s == '{') {
            const char* close = std::strchr(s, '}');
            if (close) {
                s = close;
                continue;
            }
        }
        if (*s == ' ' && (out.empty() || out.back() == ' ')) continue;
        out += *s;
    }
    if (!out.empty() && out.back() == ' ') out.pop_back();
    return out;
}

bool MatchesDialogLine(const char* rendered, const char* stored) {
    if (!rendered || !stored) return false;
    const std::string r = WithoutBraces(rendered);
    const std::string s = WithoutBraces(stored);
    return MatchesWithTokens(r.c_str(), s.c_str());
}

bool ContainsIgnoreCase(const char* haystack, const char* needle) {
    if (!haystack || !needle || !needle[0]) return false;
    for (const char* h = haystack; *h; ++h) {
        size_t i = 0;
        while (needle[i] && h[i] && Lower(h[i]) == Lower(needle[i])) ++i;
        if (!needle[i]) return true;
    }
    return false;
}

bool SameLastWord(const char* a, const char* b, size_t minLen) {
    if (!a || !b) return false;
    size_t lenA = 0, lenB = 0;
    const char* wordA = LastWord(a, lenA);
    const char* wordB = LastWord(b, lenB);
    if (lenA != lenB || lenA < minLen) return false;
    for (size_t i = 0; i < lenA; ++i) {
        if (Lower(wordA[i]) != Lower(wordB[i])) return false;
    }
    return true;
}

NoteNames NoteNamesThing(const char* note, const char* thing, size_t minLen) {
    if (ContainsIgnoreCase(note, thing)) return NoteNames::Whole;
    if (ContainsIgnoreCase(thing, note) || SameLastWord(note, thing, minLen)) {
        return NoteNames::Alike;
    }
    return NoteNames::Nothing;
}

}  // namespace acc::text

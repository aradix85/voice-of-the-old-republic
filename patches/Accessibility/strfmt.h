// Heap-backed printf → std::string. The accessibility project speaks
// localized, format-string-driven text (the strings.h table is full of
// "%s …" patterns); routing those through fixed char buffers truncated
// real announcements at arbitrary lengths (the Taris store-door exit
// dropped off the end of a 96-byte cluster label). These helpers format
// into an exactly-sized std::string so spoken/announced content is never
// silently shortened — and a malformed read shows up as visible garbage
// rather than being hidden by a clean cut, which is what we want.
//
// Header-only; no TU. Two-pass vsnprintf: measure, then fill. Also the
// ASCII case folds the code base shares.

#pragma once

#include <cstdarg>
#include <cstdio>
#include <string>

namespace acc::strfmt {

inline std::string VFormat(const char* fmt, va_list ap) {
    if (!fmt || !fmt[0]) return std::string();
    va_list ap2;
    va_copy(ap2, ap);
    int needed = std::vsnprintf(nullptr, 0, fmt, ap2);
    va_end(ap2);
    if (needed <= 0) return std::string();
    std::string out(static_cast<size_t>(needed), '\0');
    // out has `needed` chars + the guaranteed null at out[needed];
    // vsnprintf writes `needed` chars and a terminating null into that
    // null slot, which std::string permits (data()[size()] == '\0').
    std::vsnprintf(&out[0], static_cast<size_t>(needed) + 1, fmt, ap);
    return out;
}

// printf-style, unbounded. Returns "" on null/empty format or encoding
// error.
inline std::string Format(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    std::string out = VFormat(fmt, ap);
    va_end(ap);
    return out;
}

// ASCII-only case folding. <cctype>'s is locale-sensitive and takes an int;
// resrefs, module names and HTTP header names are ASCII, and the game mixes
// case within one name ("106PER" beside "106per", "po_pmha01" beside
// "po_PMHA05").
inline char AsciiLower(char c) {
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

inline char AsciiUpper(char c) {
    return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c;
}

inline std::string ToLowerAscii(std::string s) {
    for (char& c : s) c = AsciiLower(c);
    return s;
}

inline std::string ToUpperAscii(std::string s) {
    for (char& c : s) c = AsciiUpper(c);
    return s;
}

// ASCII case-insensitive substring test; false for a null or empty needle.
inline bool ContainsNoCase(const char* hay, const char* needle) {
    if (!hay || !needle || !*needle) return false;
    for (; *hay; ++hay) {
        const char* h = hay;
        const char* n = needle;
        while (*h && *n && AsciiLower(*h) == AsciiLower(*n)) { ++h; ++n; }
        if (!*n) return true;
    }
    return false;
}

}  // namespace acc::strfmt

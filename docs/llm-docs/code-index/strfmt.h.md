# strfmt.h (102 lines)

Source: `patches/Accessibility/strfmt.h`

Heap-backed printf → std::string. The accessibility project speaks localized, format-string-driven text (the strings.h table is full of "%s …" patterns); routing those through fixed char buffers truncated real announcements at arbitrary lengths (the Taris store-door exit dropped off the end of a 96-byte cluster label). These helpers format into an exactly-sized std::string so spoken/announced content is never silently shortened — and a malformed read shows up as visible garbage rather than being hidden by a clean cut, which is what we want.

Header-only; no TU. Two-pass vsnprintf: measure, then fill. Also the bounded append for the fixed buffers that remain, and the ASCII case folds the code base shares.

## Declarations (in source order)

- L23 `inline std::string VFormat(const char* fmt, va_list ap)`
- L40 `inline std::string Format(const char* fmt, ...)`: printf-style, unbounded.
- L51 `struct FixedText`: printf-style appends to a caller's fixed buffer.
- L71 `inline char AsciiLower(char c)`: ASCII-only case folding.
- L75 `inline char AsciiUpper(char c)`
- L79 `inline std::string ToLowerAscii(std::string s)`
- L84 `inline std::string ToUpperAscii(std::string s)`
- L90 `inline bool ContainsNoCase(const char* hay, const char* needle)`: ASCII case-insensitive substring test; false for a null or empty needle.

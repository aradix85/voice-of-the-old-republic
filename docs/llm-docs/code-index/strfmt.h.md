# strfmt.h (69 lines)

Source: `patches/Accessibility/strfmt.h`

Heap-backed printf → std::string. The accessibility project speaks localized, format-string-driven text (the strings.h table is full of "%s …" patterns); routing those through fixed char buffers truncated real announcements at arbitrary lengths (the Taris store-door exit dropped off the end of a 96-byte cluster label). These helpers format into an exactly-sized std::string so spoken/announced content is never silently shortened — and a malformed read shows up as visible garbage rather than being hidden by a clean cut, which is what we want.

Header-only; no TU. Two-pass vsnprintf: measure, then fill. Also the ASCII case folds the code base shares.

## Declarations (in source order)

- L21 `inline std::string VFormat(const char* fmt, va_list ap)`
- L38 `inline std::string Format(const char* fmt, ...)`: printf-style, unbounded.
- L50 `inline char AsciiLower(char c)`: ASCII-only case folding.
- L54 `inline char AsciiUpper(char c)`
- L58 `inline std::string ToLowerAscii(std::string s)`
- L63 `inline std::string ToUpperAscii(std::string s)`

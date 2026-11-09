# prism_text.cpp (93 lines)

Source: `patches/Accessibility/prism_text.cpp`

Text for the speech channels: re-encoding between the game's codepage and Prism's UTF-8, and the last line spoken (for the bridge and repeat-last).

## Declarations (in source order)

- L32 `std::string Narrow(const wchar_t* text, UINT cp)`: `text` in codepage `cp`; empty when it cannot be converted.
- L42 `std::string Recode(const char* in, UINT from, UINT to)`: `in` from codepage `from` to codepage `to`, through UTF-16.
- L56 `std::string ToUtf8(const char* gameText)`: Game codepage → UTF-8.
- L60 `std::string WideToUtf8(const wchar_t* text)`
- L64 `void CaptureSpoken(const char* text)`
- L75 `void CaptureSpokenW(const wchar_t* text)`: Folded back to the game codepage, so LastSpoken() has one encoding whichever overload produced it.
- L79 `void SetSpeechCodepage(unsigned codepage)`
- L86 `unsigned GetSpeechCodepage()`
- L88 `const char* LastSpoken()`
- L90 `void SuppressNextCapture()`

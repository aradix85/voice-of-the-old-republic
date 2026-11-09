# prism_text.cpp (99 lines)

Source: `patches/Accessibility/prism_text.cpp`

Text for the speech channels: re-encoding between the game's codepage and Prism's UTF-8, and the last line spoken (for the bridge and repeat-last).

## Declarations (in source order)

- L32 `std::string Narrow(const wchar_t* text, UINT cp)`: `text` in codepage `cp`; empty when it cannot be converted.
- L42 `std::string Recode(const char* in, UINT from, UINT to)`: `in` from codepage `from` to codepage `to`, through UTF-16.
- L56 `std::string ToUtf8(const char* gameText)`: Game codepage → UTF-8.
- L62 `std::string FromUtf8(const char* utf8)`: Prism hands voice names back in UTF-8; everything downstream of us (speech dispatch, the composed UI line) works in the game codepage.
- L66 `std::string WideToUtf8(const wchar_t* text)`
- L70 `void CaptureSpoken(const char* text)`
- L81 `void CaptureSpokenW(const wchar_t* text)`: Folded back to the game codepage, so LastSpoken() has one encoding whichever overload produced it.
- L85 `void SetSpeechCodepage(unsigned codepage)`
- L92 `unsigned GetSpeechCodepage()`
- L94 `const char* LastSpoken()`
- L96 `void SuppressNextCapture()`

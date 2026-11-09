# prism_text.cpp (131 lines)

Source: `patches/Accessibility/prism_text.cpp`

Text for the speech channels: re-encoding between the game's codepage and Prism's UTF-8, and the last line spoken (for the bridge and repeat-last).

## Declarations (in source order)

- L28 `bool ReencodeAcpToUtf8(const char* in, char* outBuf, size_t outBufSize)`: Game codepage → UTF-8.
- L61 `bool ReencodeUtf8ToGameCodepage(const char* in, char* outBuf, size_t outBufSize)`: Mirror of the above: Prism hands voice names back in UTF-8, and everything downstream of us (speech dispatch, the composed UI line) works in the game codepage.
- L90 `void CaptureSpoken(const char* text)`
- L104 `void CaptureSpokenW(const wchar_t* text)`: Wide path: fold back to the game codepage so LastSpoken() has one encoding regardless of which overload produced it.
- L117 `void SetSpeechCodepage(unsigned codepage)`
- L124 `unsigned GetSpeechCodepage()`
- L126 `const char* LastSpoken()`
- L128 `void SuppressNextCapture()`

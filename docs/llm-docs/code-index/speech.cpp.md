# speech.cpp (145 lines)

Source: `patches/Accessibility/speech.cpp`

Speech and braille through the NVDA controller client: every line goes to NVDA as speech and as a braille flash message. The client talks to NVDA over local RPC; while NVDA is not running its calls fail and the line is dropped.

## Declarations (in source order)

- L38 `bool Resolve(HMODULE lib, T& fn, const char* name)`
- L44 `std::wstring Widen(const char* text)`
- L55 `std::string Narrow(const wchar_t* text)`: To the game codepage; best-fit characters are allowed, since an approximation beats losing the line.
- L65 `void Capture(const char* text)`
- L77 `void Send(const char* logText, const wchar_t* text, bool interrupt)`: The patch log carries one canonical record of what NVDA was asked to say, in order; "[!]" marks interrupt=true.
- L87 `bool Init()`
- L120 `void Speak(const wchar_t* text, bool interrupt)`
- L127 `void Speak(const char* text, bool interrupt)`
- L134 `void Silence()`
- L140 `const char* LastSpoken()`
- L142 `void SuppressNextCapture()`

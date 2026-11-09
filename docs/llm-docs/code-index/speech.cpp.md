# speech.cpp (162 lines)

Source: `patches/Accessibility/speech.cpp`

Speech and braille through the NVDA controller client: every line goes to NVDA as speech and as a braille flash message. The client talks to NVDA over local RPC; while NVDA is not running its calls fail and the line is dropped.

## Declarations (in source order)

- L43 `bool Resolve(HMODULE lib, T& fn, const char* name)`
- L49 `std::wstring Widen(const char* text)`
- L60 `std::string Narrow(const wchar_t* text)`: To the game codepage; best-fit characters are allowed, since an approximation beats losing the line.
- L70 `void Capture(const char* text)`
- L82 `void Send(const char* logText, const wchar_t* text, bool interrupt)`: The patch log carries one canonical record of what NVDA was asked to say, in order; "[!]" marks interrupt=true.
- L92 `bool Init()`
- L125 `void Speak(const wchar_t* text, bool interrupt)`
- L132 `void Speak(const char* text, bool interrupt)`
- L139 `void SetSpeechCodepage(unsigned codepage)`
- L145 `unsigned GetSpeechCodepage()`
- L147 `void Silence()`
- L153 `const char* LastSpoken()`
- L155 `void SuppressNextCapture()`
- L157 `void Shutdown()`

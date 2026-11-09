# speech.h (58 lines)

Source: `patches/Accessibility/speech.h`

Speech and braille through NVDA, via its controller client (nvdaControllerClient32.dll in the patches folder, loaded at runtime). Without NVDA running, or without the DLL, the mod is silent.

Init MUST NOT run inside DllMain (LoadLibrary under the loader lock).

## Declarations (in source order)

- L12 `bool Init()`: Loads the controller client once; true when it is there.
- L16 `void Speak(const wchar_t* text, bool interrupt)`: One line to NVDA, spoken and shown on the braille display.
- L20 `void Speak(const char* text, bool interrupt)`: Narrow overload — text in GetSpeechCodepage().
- L31 `void SetSpeechCodepage(unsigned codepage)`: Codepage of every narrow string on the way to NVDA — both our own tables and engine CExoString text.
- L32 `unsigned GetSpeechCodepage()`
- L35 `void Silence()`: Cancel what NVDA is saying.
- L46 `const char* LastSpoken()`: ---- Last-utterance capture -------------------------------------------- Every Speak records its text here, so a hotkey can act on "the thing I just heard" without each subsystem keeping its own copy.
- L52 `void SuppressNextCapture()`: One-shot: the next Speak call does not overwrite LastSpoken().
- L55 `void Shutdown()`: No more speech for the rest of the process (DLL detach).

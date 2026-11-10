# speech.h (52 lines)

Source: `patches/Accessibility/speech.h`

Speech and braille through NVDA, via its controller client (nvdaControllerClient32.dll in the patches folder, loaded at runtime). Without NVDA running, or without the DLL, the mod is silent.

Init MUST NOT run inside DllMain (LoadLibrary under the loader lock).

## Declarations (in source order)

- L12 `void EnsureSpeechInitialized()`: Loads speech on the first hook that fires (DllMain cannot, see above) and greets with the mod version; later calls do nothing.
- L21 `bool Init()`: Loads the controller client once; true when it is there.
- L25 `void Speak(const wchar_t* text, bool interrupt)`: One line to NVDA, spoken and shown on the braille display.
- L29 `void Speak(const char* text, bool interrupt)`: Narrow overload — text in kGameCodepage.
- L32 `void Silence()`: Cancel what NVDA is saying.
- L43 `const char* LastSpoken()`: ---- Last-utterance capture -------------------------------------------- Every Speak records its text here, so a hotkey can act on "the thing I just heard" without each subsystem keeping its own copy.
- L49 `void SuppressNextCapture()`: One-shot: the next Speak call does not overwrite LastSpoken().

Also 1 constants or variables at namespace scope.

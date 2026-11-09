# prism.h (62 lines)

Source: `patches/Accessibility/prism.h`

Speech bridge — wraps the Prism C ABI.

prism.dll resolves lazily via LoadLibrary so a missing/broken install degrades to silent mode. Init MUST NOT run inside DllMain — Prism loads COM + screen-reader driver DLLs, both unsafe under the loader lock.

## Declarations (in source order)

- L12 `bool Init()`: Idempotent.
- L15 `bool IsAvailable()`: Speak/Silence are no-ops when this is false.
- L18 `void Speak(const wchar_t* text, bool interrupt)`: Speech and braille via the user's screen reader.
- L22 `void Speak(const char* text, bool interrupt)`: Narrow overload — text in GetSpeechCodepage().
- L33 `void SetSpeechCodepage(unsigned codepage)`: Codepage of every narrow string on the way to the screen reader — both our own tables and engine CExoString text.
- L34 `unsigned GetSpeechCodepage()`
- L37 `void Silence()`: Cancel in-progress speech.
- L48 `const char* LastSpoken()`: ---- Last-utterance capture -------------------------------------------- Every Speak that actually reaches the screen reader records its text here, so a hotkey can act on "the thing I just heard" without each subsyste...
- L54 `void SuppressNextCapture()`: One-shot: the next Speak call does not overwrite LastSpoken().
- L59 `void Shutdown()`: Drops IsAvailable() flag; Prism context stays — tearing it down from DLL_PROCESS_DETACH is unsafe under the loader lock.

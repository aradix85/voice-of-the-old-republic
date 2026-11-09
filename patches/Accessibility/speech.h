// Speech and braille through NVDA, via its controller client
// (nvdaControllerClient32.dll in the patches folder, loaded at runtime).
// Without NVDA running, or without the DLL, the mod is silent.
//
// Init MUST NOT run inside DllMain (LoadLibrary under the loader lock).

#pragma once

namespace acc::speech {

// Loads the controller client once; true when it is there. Safe to call again.
bool Init();

// One line to NVDA, spoken and shown on the braille display. `interrupt`
// first cancels what NVDA is still saying.
void Speak(const wchar_t* text, bool interrupt);

// Narrow overload — text in GetSpeechCodepage(). Right entry point for our
// own tables and CExoString-sourced text.
void Speak(const char* text, bool interrupt);

// Codepage of every narrow string on the way to NVDA — both our own tables and
// engine CExoString text. Defaults to CP_ACP, which is right for en/de/fr/it/es
// (all Windows-1252). Must be pinned explicitly for languages whose codepage
// differs from the player's OS codepage: a Russian install is Windows-1251 even
// on a German or English Windows, where CP_ACP is 1252 and would garble every
// Cyrillic byte; Polish is 1250.
//
// Set once from the detected game language at startup; a 0 argument is
// ignored. Safe to call before Init().
void SetSpeechCodepage(unsigned codepage);
unsigned GetSpeechCodepage();

// Cancel what NVDA is saying.
void Silence();

// ---- Last-utterance capture --------------------------------------------
//
// Every Speak records its text here, so a hotkey can act on "the thing I just
// heard" without each subsystem keeping its own copy. See speech_clipboard.cpp
// (Ctrl+R).
//
// Returns the text in the *game* codepage — widen with GetSpeechCodepage().
// Never null; "" when nothing has been spoken yet. Stable until the next
// utterance.
const char* LastSpoken();

// One-shot: the next Speak call does not overwrite LastSpoken(). For the
// mod's own meta-announcements ("copied to clipboard") which would otherwise
// become the very text the user wanted to act on. Set it immediately before
// the speak call so exactly that call consumes it.
void SuppressNextCapture();

// No more speech for the rest of the process (DLL detach).
void Shutdown();

}  // namespace acc::speech

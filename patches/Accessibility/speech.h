// Speech and braille through NVDA, via its controller client
// (nvdaControllerClient32.dll in the patches folder, loaded at runtime).
// Without NVDA running, or without the DLL, the mod is silent.
//
// Init MUST NOT run inside DllMain (LoadLibrary under the loader lock).

#pragma once

namespace acc::speech {

// Codepage of every narrow string on the way to NVDA: the English game's
// text (CExoString) and our own table.
constexpr unsigned kGameCodepage = 1252;

// Loads the controller client once; true when it is there. Safe to call again.
bool Init();

// One line to NVDA, spoken and shown on the braille display. `interrupt`
// first cancels what NVDA is still saying.
void Speak(const wchar_t* text, bool interrupt);

// Narrow overload — text in kGameCodepage. Right entry point for our own
// table and CExoString-sourced text.
void Speak(const char* text, bool interrupt);

// Cancel what NVDA is saying.
void Silence();

// ---- Last-utterance capture --------------------------------------------
//
// Every Speak records its text here, so a hotkey can act on "the thing I just
// heard" without each subsystem keeping its own copy. See speech_clipboard.cpp
// (Ctrl+R).
//
// Returns the text in kGameCodepage.
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

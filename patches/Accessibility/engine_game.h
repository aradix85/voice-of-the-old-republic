// Which game are we injected into?
//
// The mod supports one executable per game: KOTOR 1 version 1.0.3 (Steam, GOG
// and the GOG-equivalent CD re-pack are the same code) and KOTOR 2 as Aspyr
// rebuilt it in 2015 (Steam). The patch manager's version gate has checked the
// executable's hash before this DLL is loaded; here the two only have to be
// told apart.
//
// That is done by the PE link timestamp of the game's image (not this DLL's):
// one header walk of mapped memory, no file I/O, no allocation, no dependency
// on another global's initialisation. It has to be that cheap because the
// engine-address constants that ask are themselves initialised at load time,
// in unspecified order across translation units.

#pragma once

namespace acc::game {

enum class Title {
    Kotor1,
    Kotor2,
    Unknown,
};

// Resolved once and cached for the process lifetime.
Title CurrentTitle();

// An unknown executable is neither, so `!IsKotor1()` is NOT `IsKotor2()`:
// gate on the one you mean.
inline bool IsKotor1() { return CurrentTitle() == Title::Kotor1; }
inline bool IsKotor2() { return CurrentTitle() == Title::Kotor2; }

// For logs: "kotor1", "kotor2" or "unknown".
const char* TitleName();

// Default-deny gate for hook handlers during the KOTOR 2 port.
//
// Why a gate is needed before a single KOTOR 2 hook may be installed: a handler
// is not self-contained. OnSetActiveControl, for instance, calls
// IsLoadingSaveGame, which dispatches through CServerExoApp::GetLoadFromSaveGame
// - an address that resolves to 0 on KOTOR 2. Installing that hook without a
// gate calls a null pointer on the first focus change.
//
// Usage, as the first line of every hook handler:
//
//     if (!acc::game::HandlerEnabled()) return;
//
// It is a no-op on KOTOR 1 and blocks on KOTOR 2. A handler is cleared for
// KOTOR 2 by replacing that call with an explicit branch, once its whole
// read/call chain has been ported - not before.
inline bool HandlerEnabled() { return !IsKotor2(); }

}  // namespace acc::game

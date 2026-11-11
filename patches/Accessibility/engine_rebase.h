// Engine addresses, one per game.
//
// An engine address is written at its declaration with its value in each game
// that has it: KOTOR 1 version 1.0.3 and KOTOR 2 (Steam, Aspyr 2015). KOTOR 2
// is a recompile, so a KOTOR 1 address is never valid there.
//
// An address that is not known for the running game is 0. A call through it
// faults at address 0, which a crash dump shows at once; a stale address would
// land in unrelated code and corrupt state first. Guard such call sites with
// Ok(). That is not what makes the other game safe, though: code that exists
// for one game only is gated on acc::game::IsKotor1() / IsKotor2() before it
// runs at all.
//
// These run from static initialisers; see engine_game.h for why that is safe.
//
// tools/analysis/call_convention.py and engine_twins.py read these calls from
// the sources: keep an address a literal inside the call.

#pragma once

#include <cstdint>

#include "engine_game.h"

namespace acc::addr {

inline bool Ok(uintptr_t va) { return va != 0; }

// A function or .rdata address known in both games.
inline uintptr_t Pick(uintptr_t kotor1Va, uintptr_t kotor2Va) {
    return acc::game::IsKotor2() ? kotor2Va : kotor1Va;
}

// The same for a .data global; named apart so that data and code can be told
// apart at the declaration.
inline uintptr_t PickGlobal(uintptr_t kotor1Va, uintptr_t kotor2Va) {
    return Pick(kotor1Va, kotor2Va);
}

// A KOTOR 1 address whose KOTOR 2 twin has not been looked for yet: a function
// or .rdata address, and a .data global.
inline uintptr_t R(uintptr_t kotor1Va) { return Pick(kotor1Va, 0); }
inline uintptr_t TodoGlobal(uintptr_t kotor1Va) { return Pick(kotor1Va, 0); }

// Checked, and the other game has no such thing. At run time the same as an
// address not looked for; the name keeps "does not exist" apart from "to do".
inline uintptr_t Kotor1Only(uintptr_t kotor1Va) { return Pick(kotor1Va, 0); }
inline uintptr_t Kotor2Only(uintptr_t kotor2Va) { return Pick(0, kotor2Va); }

}  // namespace acc::addr

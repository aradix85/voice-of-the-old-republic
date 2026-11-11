# engine_rebase.h (51 lines)

Source: `patches/Accessibility/engine_rebase.h`

Engine addresses, one per game.

An engine address is written at its declaration with its value in each game that has it: KOTOR 1 version 1.0.3 and KOTOR 2 (Steam, Aspyr 2015). KOTOR 2 is a recompile, so a KOTOR 1 address is never valid there.

An address that is not known for the running game is 0. A call through it faults at address 0, which a crash dump shows at once; a stale address would land in unrelated code and corrupt state first. Guard such call sites with Ok(). That is not what makes the other game safe, though: code that exists for one game only is gated on acc::game::IsKotor1() / IsKotor2() before it runs at all.

These run from static initialisers; see engine_game.h for why that is safe.

tools/analysis/call_convention.py and engine_twins.py read these calls from the sources: keep an address a literal inside the call.

## Declarations (in source order)

- L27 `inline bool Ok(uintptr_t va)`
- L30 `inline uintptr_t Pick(uintptr_t kotor1Va, uintptr_t kotor2Va)`: A function or .rdata address known in both games.
- L36 `inline uintptr_t PickGlobal(uintptr_t kotor1Va, uintptr_t kotor2Va)`: The same for a .data global; named apart so that data and code can be told apart at the declaration.
- L42 `inline uintptr_t R(uintptr_t kotor1Va)`: A KOTOR 1 address whose KOTOR 2 twin has not been looked for yet: a function or .rdata address, and a .data global.
- L43 `inline uintptr_t TodoGlobal(uintptr_t kotor1Va)`
- L47 `inline uintptr_t Kotor1Only(uintptr_t kotor1Va)`: Checked, and the other game has no such thing.
- L48 `inline uintptr_t Kotor2Only(uintptr_t kotor2Va)`

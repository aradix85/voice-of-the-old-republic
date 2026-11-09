# engine_offsets_select.h (123 lines)

Source: `patches/Accessibility/engine_offsets_select.h`

Per-game selection for struct field offsets.

Why this exists --------------- `acc::addr::R()` solves the same problem for .text ADDRESSES, but it cannot be reused here, and the reason is worth stating because it is not obvious: R() is a table keyed on the reference address, which works only because addresses are globally unique. Offsets are not. `+0x4` is CAppManager.Client AND CGameObject.Id AND a dozen others, so there is no key to look up. The per-game value has to be chosen at the declaration site, where the field's identity is known.

Hence three markers rather than a table. Each constant initialises itself at load time from its own declaration, exactly as the R()-wrapped addresses do, so there is no cross-translation-unit initialisation-order problem: the only shared state is acc::game's function-local static.

Choosing a marker ----------------- Same(x)      — verified IDENTICAL in both games. constexpr, so free. Pick(k1, k2) — verified DIFFERENT. Todo(k1)     — KOTOR 1 value known, KOTOR 2 value not yet established.

`Same` and `Todo` look the same from KOTOR 1 and differ only on KOTOR 2, and that distinction is the entire point: "checked, and it happens to match" must not be confusable with "never checked". Do not use Same() to silence a Todo() you have not actually verified — that converts an honest gap into a silent wrong read.

`grep -c "Todo("` is the remaining-work counter for the port.

What is NOT here ---------------- engine_offsets_values.h holds constants that are not field offsets — vtable slot indices, TLK strrefs, action-type enum bytes, panel input codes. Those vary between the games along different axes (a strref differs because the TLK differs, not because a struct grew) and are handled separately.

Expected shape of the KOTOR 2 answers ------------------------------------- Measured against the 21 offsets both seeded AddressDatabases share: base and shallow classes are identical (CAppManager, CExoString, CGameObject, CSWBaseItem), while derived classes shift by a CONSTANT delta per class — every CSWSObject field by 4, every CSWSCreatureStats field by 4, both CSWSCreature fields by the same larger amount. That is field insertion near the top of a base class. So verifying two fields of a class usually carries the rest — but the delta is piecewise (identity below the insertion point, constant above it), so confirm rather than extrapolate blindly.

## Declarations (in source order)

- L71 `constexpr size_t Same(size_t both)`: Verified identical in both games.
- L74 `inline size_t Pick(size_t kotor1, size_t kotor2)`: Verified different between the games.
- L80 `inline size_t Todo(size_t kotor1)`: KOTOR 1 value known; KOTOR 2 value not yet reverse-engineered.
- L90 `inline size_t Kotor1Only(size_t kotor1)`: The field belongs to a subsystem KOTOR 2 does not have, so there is no K2 value to find — the turret minigame, for instance.
- L97 `inline size_t Kotor2Only(size_t kotor2)`: Mirror of Kotor1Only: a field only KOTOR 2 has (or that only a KOTOR-2-only code path reads).
- L111 `inline bool Ok(size_t off)`: Is this offset usable on the running game?
- L117 `inline void* Ptr(void* base, size_t off)`: Form an interior pointer, or nullptr when the offset is unported.

Also 1 constants or variables at namespace scope.

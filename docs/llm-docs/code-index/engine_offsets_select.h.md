# engine_offsets_select.h (122 lines)

Source: `patches/Accessibility/engine_offsets_select.h`

Per-game selection for struct field offsets.

The per-game value is chosen at the declaration, where the field's identity is known, as acc::addr does for addresses (engine_addr.h). Each constant initialises itself at load time from its own declaration, so there is no cross-translation-unit initialisation-order problem: the only shared state is acc::game's function-local static.

Choosing a marker ----------------- Same(x)      — verified IDENTICAL in both games. constexpr, so free. Pick(k1, k2) — verified DIFFERENT. Todo(k1)     — KOTOR 1 value known, KOTOR 2 value not yet established.

`Same` and `Todo` look the same from KOTOR 1 and differ only on KOTOR 2, and that distinction is the entire point: "checked, and it happens to match" must not be confusable with "never checked". Do not use Same() to silence a Todo() you have not actually verified — that converts an honest gap into a silent wrong read.

`grep -c "Todo("` is the remaining-work counter for the port.

What is NOT here ---------------- engine_offsets_values.h holds constants that are not field offsets — vtable slot indices, TLK strrefs, action-type enum bytes, panel input codes. Those vary between the games along different axes (a strref differs because the TLK differs, not because a struct grew) and are handled separately.

Expected shape of the KOTOR 2 answers ------------------------------------- Measured against the 21 offsets both seeded AddressDatabases share: base and shallow classes are identical (CAppManager, CExoString, CGameObject, CSWBaseItem), while derived classes shift by a CONSTANT delta per class — every CSWSObject field by 4, every CSWSCreatureStats field by 4, both CSWSCreature fields by the same larger amount. That is field insertion near the top of a base class. So verifying two fields of a class usually carries the rest — but the delta is piecewise (identity below the insertion point, constant above it), so confirm rather than extrapolate blindly.

## Declarations (in source order)

- L62 `constexpr size_t Same(size_t both)`: Verified identical in both games.
- L65 `inline size_t Pick(size_t kotor1, size_t kotor2)`: Verified different between the games.
- L71 `inline size_t Todo(size_t kotor1)`: KOTOR 1 value known; KOTOR 2 value not yet reverse-engineered.
- L81 `inline size_t Kotor1Only(size_t kotor1)`: The field belongs to a subsystem KOTOR 2 does not have, so there is no K2 value to find — the turret minigame, for instance.
- L88 `inline size_t Kotor2Only(size_t kotor2)`: Mirror of Kotor1Only: a field only KOTOR 2 has (or that only a KOTOR-2-only code path reads).
- L102 `inline bool Ok(size_t off)`: Is this offset usable on the running game?
- L108 `inline void* Ptr(void* base, size_t off)`: Form an interior pointer, or nullptr when the offset is unported.
- L117 `inline bool IsAt(void* base, size_t off, void* object)`: `object` is the one embedded at base+off.

Also 1 constants or variables at namespace scope.

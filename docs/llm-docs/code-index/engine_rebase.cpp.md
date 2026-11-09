# engine_rebase.cpp (260 lines)

Source: `patches/Accessibility/engine_rebase.cpp`

(From engine_rebase.h.) Engine-address rebasing for non-reference builds of swkotor.exe.

Why --- Every engine address in this patch is written as its value in the reference build (Steam/GoG 1.0.3). Another build of the same BioWare source exists — the 2004-03-05 relink, shipped by both the Allard Russian translation and the official Polish LEM edition — where the functions are byte-identical but the linker placed them elsewhere (measured -320..+640 bytes, varying per function and not monotonic, so no single offset works).

Rather than duplicate ~190 constants per build, each constant is written once against the reference and passed through R(). On the reference build R() is the identity function; on a known other build it maps through a generated table (engine_rebase_table.inc, produced by `kdev sigscan`).

Scope: .text and .rdata addresses both need this; .data is byte-stable across these builds and is left alone. The .rdata half was a late discovery — the original reasoning here was that we held no bare .rdata constants, only vtable references inside functions that move with their function. That was wrong (44 of them), and on a rebased build every vtable-identity check silently failed until engine_rebase_rdata.inc was added.

Which build we are running in comes from `engine_game.h`, which owns that question for the whole patch (it also answers "which GAME", which matters far outside address handling). Detection there is a header walk of the already-mapped image — no file I/O, no allocation — so it is safe from a static initialiser and from DllMain, which matters because the constants that call R() are themselves initialised at load time.

Failure mode: an address that is NOT in the table on a rebased build returns 0 rather than the reference value. A call through a null pointer faults at address 0 and is instantly recognisable in a crash dump; jumping to a stale address lands in the middle of an unrelated function and corrupts state first. Guard such call sites with Ok().

KOTOR 2 returns 0 for EVERY address, because every constant here is a KOTOR 1 address and K2 is a recompile rather than a relink — see the Kotor2Aspyr2015 case in R(). Ok() therefore covers the handful of guarded sites, but it is NOT the mechanism that makes K2 safe: engine-touching code must be gated on acc::game::IsKotor1() before it runs at all.

## Declarations (in source order)

- L108 `uintptr_t LookupSorted(const Entry* table, size_t count, uintptr_t key)`
- L129 `uintptr_t K2(uintptr_t steamK2Va)`: Map a Steam-K2 address to the running KOTOR 2 build.
- L141 `uintptr_t R(uintptr_t referenceVa)`
- L210 `uintptr_t Pick(uintptr_t referenceVa, uintptr_t kotor2Va)`
- L215 `uintptr_t PickGlobal(uintptr_t kotor1Va, uintptr_t kotor2Va)`
- L224 `uintptr_t Kotor2Only(uintptr_t kotor2Va)`
- L231 `uintptr_t Kotor1Only(uintptr_t referenceVa)`
- L238 `uintptr_t TodoGlobal(uintptr_t kotor1Va)`
- L246 `bool IsRebased()`: "R() is not the identity function" — true for the relinks (which remap) and for KOTOR 2 (where R() resolves everything to 0).
- L257 `const char* ActiveBuildName()`

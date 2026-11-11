# baseitem_guard.cpp (90 lines)

Source: `patches/Accessibility/baseitem_guard.cpp`

Out-of-range BaseItem guard (both games).

Every item template (.uti) names a baseitems.2da row via its BaseItem field, and the engine resolves that row through one funnel: CSWBaseItemArray::GetBaseItem(int id) — K1 0x005b31a0, K2 0x006d5200. Decompiled 2026-08-25 (both games, identical shape): count at this+0, row array at this+4, 200-byte row stride; returns NULL when the table is not loaded, id < 0, or id >= count.

Several engine callers dereference the returned row without a null check — K2's GFF item loader (FUN_006014f0) null-checks a FIELD of the row one instruction after reading through the row pointer. A third-party item whose baseitems.2da row is missing from the effective Override therefore crashes the game on every load of the area containing it (Kenny's Peragus Fuel Depot crash, docs/known-issues.md).

The guard hooks the accessor mid-function, AFTER the engine's own table-loaded check has passed (the not-loaded NULL return is deliberately left untouched — startup callers may probe before the 2DA is read) and substitutes row 0 for an out-of-range id: a strange stand-in item instead of a crash. Hook mechanics differ per game because K1 is compiled optimised and K2 is not — see the hook comments in hooks.toml / kotor2.hooks.toml.

## Declarations (in source order)

- L42 `extern "C" int __cdecl OnGetBaseItem(void* thisPtr, int id)`: KOTOR 1 — hooked at 0x005b31ab with the engine's entire bounds check (TEST EAX,EAX / JL / CMP EAX,[ECX] / JGE, 8 bytes) cut and NOT replayed: the two relative jumps in the cut cannot be relocated, so this handler IS t...
- L68 `extern "C" void __cdecl OnGetBaseItemK2(void* ebp)`: KOTOR 2 — hooked at 0x006d5210, after the prologue and the engine's table-loaded check.

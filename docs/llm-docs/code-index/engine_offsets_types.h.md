# engine_offsets_types.h (94 lines)

Source: `patches/Accessibility/engine_offsets_types.h`

Engine C++ type definitions shared across the patch.

Part of the engine_offsets.h family, split 2026-07-29 (refactoring phase 2, candidate C8) along the axis that decides portability rather than along subsystem lines. The four files and what varies them:

engine_offsets_types.h      THIS FILE. Plain C++ struct layouts for engine types we read through or construct. No addresses, no offsets. engine_offsets_addresses.h  Executable addresses: .text functions and vtables (all through acc::addr::R) plus the raw .data global pointers. Upstream AddressDatabase tables `functions` and `global_pointers`. engine_offsets_fields.h     Struct field offsets. Upstream AddressDatabase table `offsets`, keyed class + member. engine_offsets_values.h     Constants that are neither: vtable slot indices, TLK strrefs, sentinels, enum bytes, panel input codes. Not in the address database at all.

engine_offsets.h remains a thin aggregator of all four, so every existing includer is unaffected.

Why this axis: on a KOTOR 2 port each group moves independently. Every address is wrong. Struct offsets are mostly wrong but not entirely - upstream's seeded K2 database already carries CAppManager|Client at the same 0x4 we use. Resource-derived values do not track the executable at all; they follow dialog.tlk and the .gui files. Splitting them apart makes the port's real risk surface countable. See archiev/refactoring/reports/phase-2-cleanup.md.

## Declarations (in source order)

- L38 `struct CExoArrayList`: CExoArrayList — engine container for arrays of pointers.
- L50 `struct Vector`: Vector — Aurora-engine 3D vector.
- L58 `inline float DistXYSq(const Vector& a, const Vector& b)`: Distances in metres.
- L64 `inline float DistXY(const Vector& a, const Vector& b)`
- L69 `inline float DistXYToSegmentSq(const Vector& p, const Vector& a, const Vector& b)`: From p to the nearest point of the segment a-b, on the floor plan.
- L81 `inline float DistXYZSq(const Vector& a, const Vector& b)`
- L90 `struct CExoString`: CExoString - the engine's string type: heap c_string plus explicit length.

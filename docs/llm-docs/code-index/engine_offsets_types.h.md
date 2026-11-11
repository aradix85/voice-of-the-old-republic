# engine_offsets_types.h (96 lines)

Source: `patches/Accessibility/engine_offsets_types.h`

Engine C++ type definitions shared across the patch.

The engine_offsets.h family is split by what decides portability between the two games:

engine_offsets_types.h      This file. Plain C++ struct layouts for engine types we read through or construct, and the distances between positions. No addresses, no offsets. engine_offsets_addresses.h  Executable addresses: functions, vtables and .data global pointers, through acc::addr. engine_offsets_fields.h     Struct field offsets. engine_offsets_values.h     Constants that are neither: vtable slot indices, TLK strrefs, sentinels, enum bytes, panel input codes.

engine_offsets.h includes all four.

On KOTOR 2 each group moves on its own: every address differs, struct offsets mostly differ, and resource-derived values follow dialog.tlk and the .gui files, not the executable.

## Declarations (in source order)

- L29 `struct CExoArrayList`: CExoArrayList — engine container for arrays of pointers.
- L41 `struct Vector`: Vector — Aurora-engine 3D vector.
- L49 `inline float DistXYSq(const Vector& a, const Vector& b)`: Distances in metres.
- L55 `inline float DistXY(const Vector& a, const Vector& b)`
- L60 `inline float DistXYToSegmentSq(const Vector& p, const Vector& a, const Vector& b)`: From p to the nearest point of the segment a-b, on the floor plan.
- L72 `inline float DistXYZSq(const Vector& a, const Vector& b)`
- L78 `inline int WholeMetres(float metres)`: A distance as the whole metres the player is told.
- L83 `inline int WholeMetresAtLeastOne(float metres)`: The same for something that is not where the player stands: never 0.
- L92 `struct CExoString`: CExoString - the engine's string type: heap c_string plus explicit length.
